#include "malloc_bonus.h"

heap_t global_heap = {PTHREAD_MUTEX_INITIALIZER, 
                      PTHREAD_MUTEX_INITIALIZER, 
                      PTHREAD_MUTEX_INITIALIZER,
                      0, NULL, NULL, NULL, NULL, NULL, NULL};
pthread_mutex_t heap_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t g_malloc_mutex;

// static pthread_once_t  g_mutex_once = PTHREAD_ONCE_INIT;

// static void init_mutex(void) {
//     pthread_mutexattr_t attr;
//     pthread_mutexattr_init(&attr);
//     pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
//     pthread_mutex_init(&g_malloc_mutex, &attr);
//     pthread_mutexattr_destroy(&attr);
// }

// void lock_heap(void)   { pthread_once(&g_mutex_once, init_mutex); pthread_mutex_lock(&g_malloc_mutex); }
// void unlock_heap(void) { pthread_mutex_unlock(&g_malloc_mutex); }

// #define LOCK()   lock_heap()
// #define UNLOCK() unlock_heap()


void print_pointer_in_binary(void *ptr){
    u_int64_t p = (u_int64_t)ptr;
    for(int i = 63; i >= 0; i--){
        printf("%ld", (p >> i) & 1);
        if(i % 8 == 0) printf(" ");
    }
    printf(" %p\n", ptr);
}

int create_zone(size_t block_size) {
    DBG("creating new zone with block_size=%zu", block_size);

    void *base = mmap(NULL, sizeof(zone_t) + (block_size + sizeof(void *)) * BLOCKS_PER_ZONE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
        return 1;

    zone_t *zone = (zone_t *)base;

    zone->base = base;
    if(block_size == TINY_BLOCK_SIZE)
        zone->type = TINY_BLOCK;
    if(block_size == SMALL_BLOCK_SIZE)
        zone->type = SMALL_BLOCK;
    zone->block_size = block_size;
    ft_memset(zone->bitmap, 0xFF, sizeof(zone->bitmap));
    if (BLOCKS_PER_ZONE % 8 != 0)
        zone->bitmap[BLOCKS_PER_ZONE / 8] = (1 << (BLOCKS_PER_ZONE % 8)) - 1;
    zone->free_count = BLOCKS_PER_ZONE;
    zone->next = NULL;
    zone->prev = NULL;

    for(int i = 0; i < BLOCKS_PER_ZONE; i++){
        size_t offset = sizeof(zone_t) + i * (sizeof(void *) + zone->block_size);
        void **meta = (void **)((char *)zone + offset);
        *meta = (void *)zone;
    }
    zone->memory = (void *)((char *)zone + sizeof(zone_t) + sizeof(void *));

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
    block_t *block = (block_t *)mmap(NULL, sizeof(block_t) + sizeof(void *) + size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(block == MAP_FAILED){
        perror("mmap");
        return NULL;
    }
    block->size = size;
    block->type = LARGE_BLOCK;
    block->free = 1;
    void **meta = (void **)((char *)block + sizeof(block_t));
    *meta = (void *)block;
    return block;
}

int setup_global_heap(){
    DBG("setup_global_heap start");
    if (create_zone(TINY_BLOCK_SIZE) != 0)
        return 1;
    if (create_zone(SMALL_BLOCK_SIZE) != 0)
        return 1;
    global_heap.initialized = 1;
    DBG("setup_global_heap done initialized=%d tiny=%p small=%p", global_heap.initialized, (void *)global_heap.free_tiny_zones, (void *)global_heap.free_small_zones);
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
                if (bit_index >= BLOCKS_PER_ZONE)
                    return -1;
                DBG("find_set_bit found bit %d in byte %d", bit_index, i);
                return bit_index;
            }
        }
    }
    DBG("find_set_bit no free block found");
    return -1;  // No free block found
}

void *calculate_zone_block_address(zone_t *zone, int block_number){
    size_t offset = sizeof(zone_t) + block_number * (sizeof(void *) + zone->block_size) + sizeof(void *);
    void *ptr = (void *)((char *)zone + offset);
    DBG("calculate_memory_address zone=%p block_number=%d offset=%zu addr=%p",
        zone, block_number, offset, ptr);
    return ptr;
}



void *find_free_block(zone_t *zone, size_t size){
    if (!zone) {
        DBG("find_free_block called with NULL zone");
        return NULL;
    }
    int set_bit = find_set_bit(zone);
    if (set_bit == -1) {
        return NULL;
    }
    zone->bitmap[set_bit / 8] &= ~(1 << (set_bit % 8));
    zone->requested_sizes[set_bit] = size;
    zone->free_count--;
    DBG("find_free_block zone=%p block_size=%zu set_bit=%d free_count=%d bitmap_byte[%d]=0x%02x", zone, zone->block_size, set_bit, zone->free_count, set_bit / 8, zone->bitmap[set_bit / 8]);
    if(zone->free_count == 0){
        DBG("find_free_block moving zone %p to full list", zone);
        if(zone->block_size == TINY_BLOCK_SIZE){
            lst_detach(&global_heap.free_tiny_zones, zone);
            lst_add_back(&global_heap.full_tiny_zones, zone);
            if (create_zone(TINY_BLOCK_SIZE) != 0){
                return NULL;
            }
                
        }
        else if(zone->block_size == SMALL_BLOCK_SIZE){
            lst_detach(&global_heap.free_small_zones, zone);
            lst_add_back(&global_heap.full_small_zones, zone);
            if (create_zone(SMALL_BLOCK_SIZE) != 0){
                return NULL;
            }
        }
    }
    void *block_ptr = calculate_zone_block_address(zone, set_bit);
    return block_ptr;
}

void *find_free_large_block(size_t size){
    block_t *block = global_heap.free_large_blocks;
    
    // where to use block mutex here ? help me to understand the concurrency issues with large blocks -> 
    while (block){
        if (block->size >= size){
            DBG("find_free_large_block reusing free block %p size=%zu for request=%zu", block, block->size, size);
            block_detach(&global_heap.free_large_blocks, block);
            block_add_back(&global_heap.full_large_blocks, block);
            return (void *)((char *)block + sizeof(block_t) + sizeof(void *));
        }

        block = block->next;
    }
    block = create_block(size);
    if (block == NULL) {
        DBG("find_free_large_block create_block failed size=%zu", size);
        return NULL;
    }
    DBG("find_free_large_block created block %p size=%zu", block, size);
    block_add_back(&global_heap.full_large_blocks, block);
    return (void *)((char *)block + sizeof(block_t) + sizeof(void *));
}

void *alloc_block(size_t size){
    DBG("alloc_block request size=%zu", size);
    if (size <= TINY_BLOCK_SIZE)
        return find_free_block(global_heap.free_tiny_zones, size);
    if(size <= SMALL_BLOCK_SIZE)
        return find_free_block(global_heap.free_small_zones, size);
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

void *malloc(size_t size){
    if (size == 0 || size > PTRDIFF_MAX) {
        DBG("malloc size=%zu rejected", size);
        return NULL;
    }
    DBG("malloc size=%zu initialized=%d", size, global_heap.initialized);
    if (!global_heap.initialized) {
        pthread_mutex_lock(&heap_mutex);
        if (!global_heap.initialized) {
            if (setup_global_heap() != 0) {
                DBG("setup_global_heap failed");
                pthread_mutex_unlock(&heap_mutex);
                return NULL;
            }
        }
        pthread_mutex_unlock(&heap_mutex);
    }
    type_t type = size <= TINY_BLOCK_SIZE ? TINY_BLOCK :
                  size <= SMALL_BLOCK_SIZE ? SMALL_BLOCK : LARGE_BLOCK;
    lock_list(type);
    void *ptr = alloc_block(size);
    unlock_list(type);
    DBG("malloc returning ptr=%p for size=%zu", ptr, size);
    return ptr;
}

static void free_ptr(void *ptr)
{
    void    *meta_ptr;
    type_t  type;

    meta_ptr = *((void **)((char *)ptr - sizeof(void *)));
    type = ((block_t *)meta_ptr)->type;
    if (type == TINY_BLOCK || type == SMALL_BLOCK) {
        zone_t  *zone;
        size_t  block_number;
        zone_t  **free_list;

        zone = (zone_t *)meta_ptr;
        block_number = ((uintptr_t)ptr - sizeof(void *) - (uintptr_t)zone - sizeof(zone_t))
            / (sizeof(void *) + zone->block_size);
        DBG("free_ptr zone=%p block_number=%zu", zone, block_number);
        zone->bitmap[block_number / 8] |= (1 << (block_number % 8));
        zone->free_count++;
        DBG("free_ptr bitmap[%zu]=0x%02x free_count=%d",
            block_number / 8, zone->bitmap[block_number / 8], zone->free_count);
        if (zone->free_count == BLOCKS_PER_ZONE) {
            DBG("free_ptr zone %p fully empty, reclaiming", zone);
            free_list = (zone->type == TINY_BLOCK)
                ? &global_heap.free_tiny_zones
                : &global_heap.free_small_zones;
            if (zone->prev != NULL || zone->next != NULL) {
                lst_detach(free_list, zone);
                if (munmap(zone->base, sizeof(zone_t) + (zone->block_size + sizeof(void *)) * BLOCKS_PER_ZONE) == -1) {
                    DBG("free_ptr munmap failed");
                }
            }
        } else if (zone->free_count == 1) {
            DBG("free_ptr moving zone %p back to free list", zone);
            if (zone->type == TINY_BLOCK) {
                lst_detach(&global_heap.full_tiny_zones, zone);
                lst_add_back(&global_heap.free_tiny_zones, zone);

            } else if (zone->type == SMALL_BLOCK) {
                lst_detach(&global_heap.full_small_zones, zone);
                lst_add_back(&global_heap.free_small_zones, zone);
            }
        }
    } else {
        block_t *block;

        block = (block_t *)meta_ptr;
        DBG("free_ptr large block %p size=%zu", block, block->size);
        block_detach(&global_heap.full_large_blocks, block);
        if (block->size > SMALL_BLOCK_SIZE * BLOCKS_PER_ZONE) {
            munmap(block, sizeof(block_t) + sizeof(void *) + block->size);
            return;
        }
        block->free = 1;
        block_add_back(&global_heap.free_large_blocks, block);
    }
}

void free(void *ptr)
{
    if (!ptr) {
        DBG("free called with NULL pointer");
        return;
    }
    DBG("free called with ptr=%p", ptr);
    void *meta_ptr = *((void **)((char *)ptr - sizeof(void *)));
    /* On x86_64 Linux all our mmap regions live above 4 GB.
       Anything below that is a glibc-internal brk allocation — skip it. */
    if ((uintptr_t)meta_ptr < (uintptr_t)0x100000000UL) {
        DBG("free: not our allocation ptr=%p meta=%p, skipping", ptr, meta_ptr);
        return;
    }
    type_t type = ((block_t *)meta_ptr)->type;
    if (type != TINY_BLOCK && type != SMALL_BLOCK && type != LARGE_BLOCK) {
        DBG("free: invalid type %d for ptr=%p, skipping", type, ptr);
        return;
    }
    lock_list(type);
    free_ptr(ptr);
    unlock_list(type);
}

void *realloc(void *ptr, size_t size)
{
    void    *meta_ptr;
    type_t  old_type;
    type_t  new_type;
    size_t  current_size;
    void    *new_ptr;

    if (!ptr) {
        DBG("realloc called with NULL pointer, delegating to malloc size=%zu", size);
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    DBG("realloc called with ptr=%p size=%zu", ptr, size);
    meta_ptr = *((void **)((char *)ptr - sizeof(void *)));
    old_type = ((block_t *)meta_ptr)->type;
    if (old_type == TINY_BLOCK || old_type == SMALL_BLOCK) {
        current_size = ((zone_t *)meta_ptr)->block_size;
        if (size <= current_size) {
            DBG("realloc size %zu fits in zone block_size %zu, returning same ptr", size, current_size);
            return ptr;
        }
    } else {
        current_size = ((block_t *)meta_ptr)->size;
        if (size <= current_size) {
            DBG("realloc size %zu fits in large block size %zu, returning same ptr", size, current_size);
            return ptr;
        }
    }
    new_type = size <= TINY_BLOCK_SIZE ? TINY_BLOCK :
               size <= SMALL_BLOCK_SIZE ? SMALL_BLOCK : LARGE_BLOCK;
    lock_list(new_type);
    new_ptr = alloc_block(size);
    unlock_list(new_type);
    if (new_ptr == NULL) {
        DBG("realloc failed to allocate new block for size=%zu", size);
        return NULL;
    }
    ft_memcpy(new_ptr, ptr, current_size);
    DBG("realloc new=%p size=%zu old=%p", new_ptr, size, ptr);
    lock_list(old_type);
    free_ptr(ptr);
    unlock_list(old_type);
    return new_ptr;
}
