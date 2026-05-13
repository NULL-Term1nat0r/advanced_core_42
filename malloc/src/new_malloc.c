#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <valgrind/memcheck.h>

#define DEBUG 1

#if DEBUG
# define DBG(fmt, ...) \
    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
# define DBG(fmt, ...)
#endif

#define TINY_BLOCK_SIZE 64
#define SMALL_BLOCK_SIZE 1024
#define BLOCKS_PER_ZONE 100

#define NUM_BYTES ((100 + 7) / 8)

#define ALIGN  (1 <<  20)
#define ALIGN_MASK  (~(ALIGN - 1))   // 0xFFFFFFFFFFFFC000

typedef enum {
    TINY_BLOCK,
    SMALL_BLOCK,
    LARGE_BLOCK
} type_t;

typedef struct zone {
    void *base;
    type_t type;
    size_t block_size;
    unsigned char bitmap[NUM_BYTES];
    void *memory;
    int free_count;
    struct zone *next;
    struct zone *prev;
} zone_t;

typedef struct block{
    size_t size;
    type_t type;
    int free;
    void *memory;
    struct block *next;
    struct block *prev;
} block_t;

typedef struct heap {
    int initialized;
    zone_t *free_tiny_zones;
    zone_t *free_small_zones;
    zone_t *full_tiny_zones;
    zone_t *full_small_zones;
    block_t *free_large_blocks;
    block_t *full_large_blocks;

} heap_t;

heap_t global_heap = {0, NULL, NULL, NULL, NULL, NULL, NULL};

#define TYPE_SHIFT_BIT 60
#define TYPE_MASK  (0x7ULL << TYPE_SHIFT_BIT)   // bits 60
#define TAG_SHIFT_BIT 53
#define TAG_MASK  (0x7FULL << TAG_SHIFT_BIT)   // bits 60-62

u_int64_t type_to_tag(type_t type) {
    if(type == TINY_BLOCK)
        return 0x0;
    if(type == SMALL_BLOCK)
        return 0x1;
    if(type == LARGE_BLOCK)
        return 0x2;
    return 0x7; // Invalid type
}

static inline void *ptr_pack(void *ptr, u_int64_t tag, type_t type) {
    u_int64_t p = (u_int64_t)ptr;

    // clear existing tag bits first
    p &= ~TAG_MASK;

    // insert tag
    p |= ((tag) << TAG_SHIFT_BIT);
    // insert type
    p |= ((type_to_tag(type)) << TYPE_SHIFT_BIT);

    return (void *)p;
}

static inline void *ptr_unpack(void *packed) {
    u_int64_t p = (u_int64_t)packed;

    // clear tag bits
    p &= ~TAG_MASK;
    // clear type bits
    p &= ~TYPE_MASK;

    return (void *)p;
}

static inline u_int64_t ptr_get_tag(void *packed) {
    return (((u_int64_t)packed) >> TAG_SHIFT_BIT) & 0x7FULL;
}

static inline type_t ptr_get_type(void *packed) {
    u_int64_t type = (((u_int64_t)packed) >> TYPE_SHIFT_BIT) & 0x7ULL;
    if(type == 0x0)
        return TINY_BLOCK;
    if(type == 0x1)
        return SMALL_BLOCK;
    if(type == 0x2)
        return LARGE_BLOCK;
    return -1; // Invalid type
}

void print_pointer_in_binary(void *ptr){
    u_int64_t p = (u_int64_t)ptr;
    for(int i = 63; i >= 0; i--){
        printf("%ld", (p >> i) & 1);
        if(i % 8 == 0) printf(" ");
    }
    printf(" %p\n", ptr);
}


void lst_add_back(zone_t **head, zone_t *new_lst){
    if (head == NULL || new_lst == NULL) {
        DBG("lst_add_back invalid args head=%p new_lst=%p", (void *)head, (void *)new_lst);
        return;
    }
    new_lst->next = NULL;
    if (*head == NULL) {
        DBG("lst_add_back creating new head %p", new_lst);
        new_lst->prev = NULL;
        *head = new_lst;
        return;
    }
    zone_t *last = *head;
    while (last->next)
        last = last->next;
    DBG("lst_add_back appending %p after %p", new_lst, last);
    last->next = new_lst;
    new_lst->prev = last;
}

void block_add_back(block_t **head, block_t *new_block){
    if (head == NULL || new_block == NULL) {
        DBG("block_add_back invalid args head=%p new_block=%p", (void *)head, (void *)new_block);
        return;
    }
    new_block->next = NULL;
    if (*head == NULL) {
        DBG("block_add_back creating new head %p", new_block);
        new_block->prev = NULL;
        *head = new_block;
        return;
    }
    block_t *last = *head;
    while (last->next)
        last = last->next;
    DBG("block_add_back appending %p after %p", new_block, last);
    last->next = new_block;
    new_block->prev = last;
}

void lst_detach(zone_t **head, zone_t *lst){
    if (!head || !lst) {
        DBG("lst_detach invalid args head=%p lst=%p", (void *)head, (void *)lst);
        return;
    }
    if (lst->prev)
        lst->prev->next = lst->next;
    else
        *head = lst->next;
    if (lst->next)
        lst->next->prev = lst->prev;
    lst->next = NULL;
    lst->prev = NULL;
    DBG("lst_detach removed %p from list head now %p", lst, (void *)*head);
}

void block_detach(block_t **head, block_t *block){
    if (!head || !block) {
        DBG("block_detach invalid args head=%p block=%p", (void *)head, (void *)block);
        return;
    }
    if (block->prev)
        block->prev->next = block->next;
    else
        *head = block->next;
    if (block->next)
        block->next->prev = block->prev;
    block->next = NULL;
    block->prev = NULL;
    DBG("block_detach removed %p from list head now %p", block, (void *)*head);
}

zone_t *block_to_zone(void *block) {
    return (zone_t *)(((u_int64_t)block) & ~(ALIGN - 1));
}

int create_zone(size_t block_size) {
    DBG("creating new zone with block_size=%zu", block_size);
    size_t request_size = ALIGN * 2;

    void *base = mmap(NULL, request_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
        return 1;

    u_int64_t aligned = ((u_int64_t)base + ALIGN - 1) & ALIGN_MASK;

    zone_t *zone = (zone_t *)aligned;

    zone->base = base;
    if(block_size == TINY_BLOCK_SIZE)
        zone->type = TINY_BLOCK;
    if(block_size == SMALL_BLOCK_SIZE)
        zone->type = SMALL_BLOCK;
    zone->block_size = block_size;
    memset(zone->bitmap, 0xFF, sizeof(zone->bitmap));
    zone->memory = (void *)(zone + 1);
    zone->free_count = BLOCKS_PER_ZONE;
    zone->next = NULL;
    zone->prev = NULL;
    if (block_size == TINY_BLOCK_SIZE) {
        DBG("inserting new TINY zone %p", zone);
        lst_add_back(&global_heap.free_tiny_zones, zone);
    } else if (block_size == SMALL_BLOCK_SIZE) {
        DBG("inserting new SMALL zone %p", zone);
        lst_add_back(&global_heap.free_small_zones, zone);
    }
    return 0;
}

block_t *create_block(size_t size){
    DBG("create_block block_size=%zu", size);
    block_t *block = (block_t *)mmap(NULL, sizeof(block_t) + size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(block == MAP_FAILED){
        perror("mmap");
        return NULL;
    }
    block->size = size;
    block->type = LARGE_BLOCK;
    block->free = 1;
    block->memory = (void *)(block + 1);
    return block;
}

int setup_global_heap(){
    DBG("setup_global_heap start");
    if (create_zone(TINY_BLOCK_SIZE) != 0)
        return 1;
    if (create_zone(SMALL_BLOCK_SIZE) != 0)
        return 1;
    global_heap.initialized = 1;
    DBG("setup_global_heap done initialized=%d tiny=%p small=%p",
        global_heap.initialized,
        (void *)global_heap.free_tiny_zones,
        (void *)global_heap.free_small_zones);
    return 0;
}

int find_set_bit(zone_t *zone) {
    if (!zone) {
        DBG("find_set_bit called with NULL zone");
        return -1;
    }
    for (int i = 0; i < NUM_BYTES; i++) {
        unsigned char byte = zone->bitmap[i];
        for (int j = 0; j < 8; j++) {
            if (byte & (1 << j)) {
                int bit_index = i * 8 + j;
                DBG("find_set_bit found bit %d in byte %d", bit_index, i);
                return bit_index;
            }
        }
    }
    DBG("find_set_bit no free block found");
    return -1;  // No free block found
}

void *calculate_zone_block_address(zone_t *zone, int block_number){
    size_t offset = sizeof(zone_t) + block_number * zone->block_size;
    void *ptr = (void *)((char *)zone + offset);
    DBG("calculate_memory_address zone=%p block_number=%d offset=%zu addr=%p",
        zone, block_number, offset, ptr);
    return ptr;
}



void *find_free_block(zone_t *zone){
    if (!zone) {
        DBG("find_free_block called with NULL zone");
        return NULL;
    }
    int set_bit = find_set_bit(zone);
    if (set_bit == -1)
        return NULL;
    zone->bitmap[set_bit / 8] &= ~(1 << (set_bit % 8));
    zone->free_count--;
    DBG("find_free_block zone=%p block_size=%zu set_bit=%d free_count=%d bitmap_byte[%d]=0x%02x",
        zone, zone->block_size, set_bit, zone->free_count,
        set_bit / 8, zone->bitmap[set_bit / 8]);
    if(zone->free_count == 0){
        DBG("find_free_block moving zone %p to full list", zone);
        if(zone->block_size == TINY_BLOCK_SIZE){
            lst_detach(&global_heap.free_tiny_zones, zone);
            if (create_zone(TINY_BLOCK_SIZE) != 0)
                return NULL;
        }
        else if(zone->block_size == SMALL_BLOCK_SIZE){
            lst_detach(&global_heap.free_small_zones, zone);
            if (create_zone(SMALL_BLOCK_SIZE) != 0)
                return NULL;
        }
    }
    void *block_ptr = calculate_zone_block_address(zone, set_bit);
    void *packed_block_ptr = ptr_pack(block_ptr, set_bit, zone->type);
    return packed_block_ptr;
}

void *find_free_large_block(size_t size){
    block_t *block = create_block(size);
    if (block == NULL) {
        DBG("find_free_large_block create_block failed size=%zu", size);
        return NULL;
    }
    block->size = size;
    DBG("find_free_large_block created block %p memory=%p size=%zu", block, block->memory, size);
    block_add_back(&global_heap.full_large_blocks, block);
    void *ptr = (void *)((char *)block + sizeof(block_t));
    ptr = ptr_pack(ptr, 0, LARGE_BLOCK);
    return ptr;
}

void *alloc_block(size_t size){
    DBG("alloc_block request size=%zu", size);
    if (size <= TINY_BLOCK_SIZE)
        return find_free_block(global_heap.free_tiny_zones);
    if(size <= SMALL_BLOCK_SIZE)
        return find_free_block(global_heap.free_small_zones);
    else{
        DBG("alloc_block request large size=%zu, delegating to find_free_large_block", size);
        void *ptr = find_free_large_block(size);
        if(ptr == NULL){
            DBG("alloc_block failed to allocate large block size=%zu", size);
            return NULL;
        }
        return ptr;
    }
    DBG("alloc_block request too large size=%zu", size);
    return NULL;
}

void *ft_malloc(size_t size){
    DBG("ft_malloc size=%zu initialized=%d", size, global_heap.initialized);
    if(global_heap.initialized == 0){
        if(setup_global_heap() != 0) {
            DBG("setup_global_heap failed");
            return NULL;
        }
    }
    DBG("ft_malloc returning block for size=%zu", size);
    return alloc_block(size);
}

void ft_free(void *ptr){
    if(!ptr){
        DBG("ft_free called with NULL pointer");
        return;
    }
    DBG("ft_free called with ptr=%p", ptr);
    u_int64_t tag = ptr_get_tag(ptr);
    type_t type = ptr_get_type(ptr);
    if(type == -1){
        DBG("ft_free invalid type for ptr=%p", ptr);
        return;
    }
    if(type == TINY_BLOCK || type == SMALL_BLOCK){
        void *block_ptr = ptr_unpack(ptr);
        zone_t *zone = block_to_zone(block_ptr);
        size_t block_number = ptr_get_tag(ptr);
        DBG("ft_free freeing block in zone %p block_number=%zu", zone, block_number);
        zone->bitmap[block_number / 8] |= (1 << (block_number % 8));
        zone->free_count++;
        DBG("ft_free updated bitmap byte[%zu]=0x%02x free_count=%d",
            block_number / 8, zone->bitmap[block_number / 8], zone->free_count);
        if(zone->free_count == 1){
            DBG("ft_free moving zone %p back to free list", zone);
            if(zone->type == TINY_BLOCK){
                lst_detach(&global_heap.full_tiny_zones, zone);
                lst_add_back(&global_heap.free_tiny_zones, zone);
            }
            else if(zone->type == SMALL_BLOCK){
                lst_detach(&global_heap.full_small_zones, zone);
                lst_add_back(&global_heap.free_small_zones, zone);
            }
        }
    }
    else if(type == LARGE_BLOCK){
        void *raw_ptr = ptr_unpack(ptr);
        block_t *block = (block_t *)((char *)raw_ptr - sizeof(block_t));
        DBG("ft_free freeing large block %p memory=%p size=%zu", block, block->memory, block->size);
        // For simplicity, we won't implement coalescing or reuse of large blocks in this example
        block->free = 1;
        block_detach(&global_heap.full_large_blocks, block);
        block_add_back(&global_heap.free_large_blocks, block);
    }
    DBG("ft_free type=%d", (int)type);

}


int main(){
    // unsigned char bitmap[13];
    // memset(bitmap, 1, sizeof(bitmap));

    // int a = 5;
    // int i = 8;

    // a |= (1 << 3);
    // printf("a: %i\n", a);
    // a |= (2 << 3);
    // printf("a: %i\n", a);


    // printf("bit: %i\n", a >> 3 & 1);
    
    // printf("number: %i\n", a << 2 & 1);
    // while(i != 0){
    //     printf("bit_%i: %i | ", i, a >> i - 1 & 1);
    //     i--;
    // }
    // // printf("NEW_NUMBER; %i", a << 8 & 1);
    // char *ptr_array[101];
    // for(int i = 0; i < 101; i++){
    //     ptr_array[i] = ft_malloc(64);
    // }
    // char *ptr_2 = ft_malloc(1065);
    // for(int i = 0; i < 1065; i++){
    //     ptr_2[i] = 'l';
    // }
    // printf("ptr_2: %p\n", ptr_2);
    // printf("char_pointer_test: %s\n", ptr_2);
    // ft_free(ptr_2);

    char *ptr = ft_malloc(6000);
    ft_free(ptr);
    char *ptr_2 = ft_malloc(6000);
    ft_free(ptr_2);

    // char *ptr = ft_malloc(64);
    // char *ptr_1 = ft_malloc(64);
    // printf("char_ptr: %p", ptr);
    return 0;

}