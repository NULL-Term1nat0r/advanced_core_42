#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <valgrind/memcheck.h>

/* ---------------- DEBUG ---------------- */

#define DEBUG 0

#if DEBUG
# define DBG(fmt, ...) \
    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
# define DBG(fmt, ...)
#endif

/* ---------------- STRUCTS ---------------- */

typedef enum e_zone_type {
    TINY_ZONE,
    SMALL_ZONE,
    LARGE_ZONE
} zone_type_t;

typedef struct block {
    size_t size;
    int free;
    zone_type_t type;
    struct block *next;
    struct block *prev;
} block_t;

typedef struct zone {
    zone_type_t type;
    block_t *blocks;
    struct zone *next;
} zone_t;

typedef struct heap {
    zone_t *zones;
    block_t *large_allocations;
} heap_t;

static heap_t heap;

/* ---------------- CONFIG ---------------- */

#define TINY_MAX   64
#define SMALL_MAX  1024
#define BLOCKS_PER_ZONE 10

#define TINY_ZONE_SIZE  (BLOCKS_PER_ZONE * (TINY_MAX + sizeof(block_t)))
#define SMALL_ZONE_SIZE (BLOCKS_PER_ZONE * (SMALL_MAX + sizeof(block_t)))

/* ---------------- UTIL ---------------- */

block_t *get_last_block(block_t *head){
    block_t *current = head;
    if(head == NULL)
        return NULL;
    while(current->next != NULL)
        current = current->next;
    return current;
}

size_t align_page(size_t size)
{
    size_t page = getpagesize();
    return ((size + page - 1) / page) * page;
}

/* ---------------- BLOCKS ---------------- */

block_t *find_free_block(block_t *head)
{
    int i = 0;

    while (head)
    {
        DBG("scan block %d | ptr=%p free=%d next=%p",
            i, head, head->free, head->next);

        if (head->free)
            return head;

        head = head->next;
        i++;
    }

    DBG("find_free_block: no free block found");
    return NULL;
}

/* ---------------- ZONE INIT ---------------- */

block_t *init_zone_blocks(size_t zone_size, size_t block_size, zone_type_t type)
{
    void *zone_mem = mmap(NULL, zone_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);

    if (zone_mem == MAP_FAILED)
    {
        DBG("mmap FAILED for blocks size=%zu", zone_size);
        return NULL;
    }

    DBG("zone memory allocated at %p size=%zu", zone_mem, zone_size);

    block_t *head = (block_t *)zone_mem;
    block_t *current = head;

    for (size_t i = 0; i < BLOCKS_PER_ZONE; i++)
    {
        current->size = block_size;
        current->free = 1;
        current->type = type;

        DBG("init block %zu at %p", i, current);

        if (i < BLOCKS_PER_ZONE - 1)
        {
            current->next =
                (block_t *)((char *)current + sizeof(block_t) + block_size);
            DBG("block %zu next -> %p", i, current->next);
        }
        else
        {
            current->next = NULL;
            DBG("block %zu next -> NULL", i);
        }

        current->prev = (i == 0)
            ? NULL
            : (block_t *)((char *)current - sizeof(block_t) - block_size);

        if (current->next)
            current = current->next;
    }

    return head;
}

/* ---------------- ZONE CREATION ---------------- */

zone_t *create_zone(zone_type_t type, size_t block_size)
{
    size_t zone_size =
        align_page(BLOCKS_PER_ZONE * (block_size + sizeof(block_t)));

    zone_t *zone = mmap(NULL, sizeof(zone_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (zone == MAP_FAILED)
    {
        DBG("zone mmap FAILED type=%d", type);
        return NULL;
    }

    DBG("zone created type=%d ptr=%p", type, zone);

    zone->type = type;
    zone->blocks = init_zone_blocks(zone_size, block_size, type);
    zone->next = NULL;

    if (!zone->blocks)
    {
        DBG("zone init FAILED type=%d", type);
        return NULL;
    }

    return zone;
}

/* ---------------- HEAP INIT ---------------- */

int setup_zones(void)
{
    DBG("setup_zones() called");

    zone_t *tiny = create_zone(TINY_ZONE, TINY_MAX);
    zone_t *small = create_zone(SMALL_ZONE, SMALL_MAX);

    if (!tiny || !small)
    {
        DBG("setup_zones FAILED tiny=%p small=%p", tiny, small);
        return 0;
    }

    heap.zones = tiny;
    tiny->next = small;
    small->next = NULL;

    heap.large_allocations = NULL;

    DBG("zones linked successfully");
    return 1;
}

/* ---------------- ZONE FIND ---------------- */

zone_t *find_zone(zone_type_t type)
{
    zone_t *z = heap.zones;

    while (z)
    {
        DBG("zone check type=%d ptr=%p", z->type, z);

        if (z->type == type)
            return z;

        z = z->next;
    }

    DBG("find_zone: not found type=%d", type);
    return NULL;
}

/* ---------------- MALLOC CORE ---------------- */

void *ft_malloc(size_t size)
{
    DBG("ft_malloc size=%zu", size);

    if (!heap.zones)
    {
        DBG("heap not initialized");
        if (!setup_zones())
        {
            DBG("setup_zones FAILED");
            return NULL;
        }
    }

    zone_type_t type;
    size_t block_size;

    if (size <= TINY_MAX)
    {
        type = TINY_ZONE;
        block_size = TINY_MAX;
    }
    else if (size <= SMALL_MAX)
    {
        type = SMALL_ZONE;
        block_size = SMALL_MAX;
    }
    else
    {
        size_t total = size + sizeof(block_t);

        block_t *new_block = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (new_block == MAP_FAILED)
            return NULL;

        new_block->size = size;
        new_block->free = 0;
        new_block->type = LARGE_ZONE;

        block_t *last_block = get_last_block(heap.large_allocations);
        if (!last_block){
            heap.large_allocations = new_block;
            new_block->prev = last_block;
            new_block->next = NULL;
            
        }
        else {
            last_block->next = new_block;
            new_block->prev = last_block;
            new_block->next = NULL;
        }


        return (char *)new_block + sizeof(block_t);
    }

    DBG("mapped to zone type=%d block_size=%zu", type, block_size);

    zone_t *zone = find_zone(type);

    if (!zone || !zone->blocks)
    {

        DBG("zone or blocks NULL");
        return NULL;
    }

    block_t *block = find_free_block(zone->blocks);

    if (!block)
    {
        DBG("jumped here into free block == NULL");
        zone_t *new_zone = NULL;
        if (type == TINY_ZONE){
            new_zone = create_zone(type, TINY_MAX);
            if (!new_zone)
                return NULL;
            new_zone->type = TINY_ZONE;
            new_zone->next = NULL;
        }
        if (type == SMALL_ZONE){
            new_zone = create_zone(type, SMALL_MAX);
            if (!new_zone)
                return NULL;
            new_zone->type = SMALL_ZONE;
            new_zone->next = NULL;
        }
        zone_t *current = heap.zones;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_zone;
        void *ret = (char *)new_zone->blocks + sizeof(block_t);
        VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, 0);
        return ret;

        // DBG("no free block found");
    }

    block->free = 0;

    void *ret = (char *)block + sizeof(block_t);

    DBG("allocated ptr=%p", ret);

    VALGRIND_MALLOCLIKE_BLOCK(ret, size, 0, 0);

    return ret;
}

void ft_free(void *pointer){
    if(!pointer)
        return;
    VALGRIND_FREELIKE_BLOCK(pointer, 0);
    block_t *struct_pointer = (block_t *)((char *)pointer - sizeof(block_t));
    if (struct_pointer->type == LARGE_ZONE){
        printf("found large pointer\n");
        //check if first element in list and second element follows
        if(struct_pointer->prev == NULL && struct_pointer->next != NULL){
            heap.large_allocations = struct_pointer->next;
            struct_pointer->next->prev = NULL;
        }
        // check if element is between blocks
        if(struct_pointer->prev != NULL && struct_pointer->next != NULL){
            struct_pointer->prev->next = struct_pointer->next;
            struct_pointer->next->prev = struct_pointer->prev;
        }
        // check if block is only element in list
        if(struct_pointer->prev == NULL && struct_pointer->next == NULL){
            heap.large_allocations = NULL;
        }
        // check if is is the last element in list
        if(struct_pointer->prev != NULL && struct_pointer->next == NULL){
            struct_pointer->prev->next = NULL;
        }
        munmap(struct_pointer, struct_pointer->size + sizeof(block_t));
    }
    else{
        struct_pointer->free = 1;
    }
    DBG("free pointer address %p", struct_pointer);
    return;
}

void print_block_information(block_t *current_block){
    void *ret_pointer = (char *)current_block + sizeof(block_t);
    void *end_pointer = (char *)ret_pointer + current_block->size;
    printf("%p - %p : %li bytes\n", ret_pointer, end_pointer, current_block->size);
}

void print_zone_information(zone_t *current_zone, zone_type_t type){
    if(type == TINY_ZONE)
        printf("TINY: %p\n", current_zone->blocks);
    if(type == SMALL_ZONE)
        printf("SMALL: %p\n", current_zone->blocks);
    while(current_zone != NULL){
        if(current_zone->type == TINY_ZONE){
            block_t *current_block = current_zone->blocks;
            while(current_block != NULL){
                if(current_block->free == 0){
                    print_block_information(current_block);
                }
                current_block = current_block->next;
            }
        }
        current_zone = current_zone->next;
    }
}

void show_alloc_mem(){
    zone_t *current_zone = heap.zones;
    
    print_zone_information(current_zone, TINY_ZONE);
    print_zone_information(current_zone, SMALL_ZONE);
    block_t *current_block = heap.large_allocations;
    if(current_block != NULL)
        printf("LARGE: %p\n", current_block);
    while(current_block != NULL){
        block_t *last = get_last_block(current_block);
        print_block_information(current_block);
        current_block = current_block->next;
    }
}



/* ---------------- TEST ---------------- */

int main(void)
{
    int number_char_pointers = 10;
    char *arr[number_char_pointers];

    DBG("PROGRAM START");

    for (int i = 0; i < number_char_pointers; i++)
    {
        arr[i] = ft_malloc(60);

        if (!arr[i])
        {
            DBG("allocation failed at i=%d", i);
            return 1;
        }

        arr[i][0] = 't';
        arr[i][1] = 'e';
        arr[i][2] = 's';
        arr[i][3] = 't';
    }

    DBG("printing pointers");

    printf("%p\n", arr[1]);
    printf("%p\n", arr[2]);
    printf("%p\n", arr[3]);
    printf("%p\n", arr[4]);
    printf("%p\n", arr[5]);
    printf("%p\n", arr[6]);

    for (int i = 0; i < number_char_pointers; i++)
    {
        ft_free(arr[i]);
    }

    for (int i = 0; i < number_char_pointers; i++)
    {
        arr[i] = ft_malloc(60);

        if (!arr[i])
        {
            DBG("allocation failed at i=%d", i);
            return 1;
        }

        arr[i][0] = 't';
        arr[i][1] = 'e';
        arr[i][2] = 's';
        arr[i][3] = 't';
    }

    printf("Adresscheck\n");
    printf("%p\n", arr[1]);
    printf("%p\n", arr[2]);
    printf("%p\n", arr[3]);
    printf("%p\n", arr[4]);
    printf("%p\n", arr[5]);
    printf("%p\n", arr[6]);

    printf("%s\n", arr[6]);

    // for (int i = 0; i < number_char_pointers; i++)
    // {
    //     ft_free(arr[i]);
    // }



    printf("big allocation\n");
    void *pointer1 = ft_malloc(2000);
    void *pointer2 = ft_malloc(2000);
    void *pointer3 = ft_malloc(2000);
    void *pointer4 = ft_malloc(2000);

    
    // ft_free(pointer1);
    // ft_free(pointer2);
    ft_free(pointer4);
    
    
    show_alloc_mem();

    printf("pointer1_address: %p\n", pointer1);
    printf("pointer2_address: %p\n", pointer2);
    printf("pointer3_address: %p\n", pointer3);
    printf("pointer4_address: %p\n", pointer4);

    ft_free(pointer1);
    show_alloc_mem();

    return 0;
}