#include "malloc.h"

heap_t global_heap = {0, NULL, NULL, NULL, NULL, NULL, NULL};

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

    size_t page_size = (size_t)getpagesize();
    DBG("system page size=%zu", page_size);

    // Round up to full pages, guaranteeing at least BLOCKS_PER_ZONE blocks fit
    size_t min_size = sizeof(zone_t)
        + (BLOCKS_PER_ZONE + 7) / 8
        + BLOCKS_PER_ZONE * (sizeof(size_t) + sizeof(void *) + block_size);
    size_t total_size = ((min_size + page_size - 1) / page_size) * page_size;

    // Solve: sizeof(zone_t) + n*(sizeof(void*)+block_size+sizeof(size_t)) + ceil(n/8) <= total_size
    // => n * (8*per_block + 1) <= 8*(total_size - sizeof(zone_t))
    size_t per_block = sizeof(void *) + block_size + sizeof(size_t);
    int n_blocks = (int)(8 * (total_size - sizeof(zone_t))) / (int)(8 * per_block + 1);
    if (n_blocks > 2047)
        n_blocks = 2047;

    void *base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
        return 1;

    zone_t *zone = (zone_t *)base;
    zone->base = base;
    zone->zone_size = total_size;
    zone->number_of_blocks = n_blocks;
    zone->free_count = n_blocks;
    zone->next = NULL;
    zone->prev = NULL;
    zone->type = (block_size == TINY_BLOCK_SIZE) ? TINY_BLOCK : SMALL_BLOCK;

    // bitmap sits immediately after the zone_t header
    zone->bitmap = (unsigned char *)((char *)base + sizeof(zone_t));
    size_t bitmap_bytes = (n_blocks + 7) / 8;

    // requested_sizes sits immediately after bitmap
    zone->requested_sizes = (size_t *)(zone->bitmap + bitmap_bytes);

    // All blocks free: set bitmap to all 1s, then mask off unused trailing bits
    ft_memset(zone->bitmap, 0xFF, bitmap_bytes);
    if (n_blocks % 8 != 0)
        zone->bitmap[bitmap_bytes - 1] = (unsigned char)((1 << (n_blocks % 8)) - 1);

    // Block data (meta pointer + payload) starts after bitmap + requested_sizes
    size_t data_offset = sizeof(zone_t) + bitmap_bytes + n_blocks * sizeof(size_t);
    for (int i = 0; i < n_blocks; i++) {
        void **meta = (void **)((char *)base + data_offset + i * (sizeof(void *) + block_size));
        *meta = META_PACK(zone, i);
    }
    zone->memory = (void *)((char *)base + data_offset + sizeof(void *));

    if (block_size == TINY_BLOCK_SIZE) {
        DBG("inserting new TINY zone %p n_blocks=%d total_size=%zu", zone, n_blocks, total_size);
        lst_add_back(&global_heap.free_tiny_zones, zone);
    } else if (block_size == SMALL_BLOCK_SIZE) {
        DBG("inserting new SMALL zone %p n_blocks=%d total_size=%zu", zone, n_blocks, total_size);
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
    int num_bytes = (zone->number_of_blocks + 7) / 8;
    for (int i = 0; i < num_bytes; i++) {
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
    void *ptr = (void *)((char *)zone->memory + block_number * (sizeof(void *) + ZONE_BLOCK_SIZE(zone)));
    DBG("calculate_memory_address zone=%p block_number=%d addr=%p", zone, block_number, ptr);
    return ptr;
}



void *find_free_block(zone_t *zone, size_t size){
    if (!zone) {
        DBG("find_free_block called with NULL zone");
        return NULL;
    }
    int set_bit = find_set_bit(zone);
    if (set_bit == -1)
        return NULL;
    zone->bitmap[set_bit / 8] &= ~(1 << (set_bit % 8));
    zone->requested_sizes[set_bit] = size;
    zone->free_count--;
    DBG("find_free_block zone=%p set_bit=%d free_count=%d bitmap_byte[%d]=0x%02x",
        zone, set_bit, zone->free_count,
        set_bit / 8, zone->bitmap[set_bit / 8]);
    if(zone->free_count == 0){
        DBG("find_free_block moving zone %p to full list", zone);
        if(zone->type == TINY_BLOCK){
            lst_detach(&global_heap.free_tiny_zones, zone);
            lst_add_back(&global_heap.full_tiny_zones, zone);
            if (create_zone(TINY_BLOCK_SIZE) != 0)
                return NULL;
        }
        else if(zone->type == SMALL_BLOCK){
            lst_detach(&global_heap.free_small_zones, zone);
            lst_add_back(&global_heap.full_small_zones, zone);
            if (create_zone(SMALL_BLOCK_SIZE) != 0)
                return NULL;
        }
    }
    void *block_ptr = calculate_zone_block_address(zone, set_bit);
    return block_ptr;
}

void *find_free_large_block(size_t size){
    block_t *block = global_heap.free_large_blocks;
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
    if(global_heap.initialized == 0){
        if(setup_global_heap() != 0) {
            DBG("setup_global_heap failed");
            
            return NULL;
        }
    }
    DBG("malloc returning block for size=%zu", size);
    void *ptr = alloc_block(size);
    
    return ptr;
}

void free(void *ptr){
    if(!ptr){
        DBG("free called with NULL pointer");
        return;
    }
    
    DBG("free called with ptr=%p", ptr);
    void *meta = *((void **)((char *)ptr - sizeof(void *)));
    if (META_IS_ZONE(meta)) {
        zone_t *zone = META_ZONE(meta);
        int block_number = META_BLOCK_NUM(meta);
        DBG("free freeing block in zone %p block_number=%d", zone, block_number);
        zone->bitmap[block_number / 8] |= (1 << (block_number % 8));
        zone->free_count++;
        DBG("free updated bitmap byte[%d]=0x%02x free_count=%d",
            block_number / 8, zone->bitmap[block_number / 8], zone->free_count);
        if (zone->free_count == zone->number_of_blocks) {
            DBG("free zone %p fully empty, reclaiming", zone);
            zone_t **free_list = (zone->type == TINY_BLOCK)
                ? &global_heap.free_tiny_zones
                : &global_heap.free_small_zones;
            if (zone->prev != NULL || zone->next != NULL) {
                lst_detach(free_list, zone);
                munmap(zone->base, zone->zone_size);
                return;
            }
        } else if (zone->free_count == 1) {
            DBG("free moving zone %p back to free list", zone);
            if (zone->type == TINY_BLOCK) {
                lst_detach(&global_heap.full_tiny_zones, zone);
                lst_add_back(&global_heap.free_tiny_zones, zone);
            } else if (zone->type == SMALL_BLOCK) {
                lst_detach(&global_heap.full_small_zones, zone);
                lst_add_back(&global_heap.free_small_zones, zone);
            }
        }
    } else {
        block_t *block = (block_t *)meta;
        block_detach(&global_heap.full_large_blocks, block);
        if (block->size > SMALL_BLOCK_SIZE * BLOCKS_PER_ZONE) {
            munmap(block, sizeof(block_t) + sizeof(void *) + block->size);
            return;
        }
        block->free = 1;
        block_add_back(&global_heap.free_large_blocks, block);
    }
    
}


void *realloc(void *ptr, size_t size){
    if(!ptr){
        DBG("realloc called with NULL pointer, delegating to malloc size=%zu", size);
        return malloc(size);
    }
    
    DBG("realloc called with ptr=%p size=%zu", ptr, size);
    void *meta = *((void **)((char *)ptr - sizeof(void *)));
    size_t current_size;
    if (META_IS_ZONE(meta)) {
        current_size = ZONE_BLOCK_SIZE(META_ZONE(meta));
        if (size <= current_size) {
            DBG("realloc size %zu fits in zone block_size %zu, returning same pointer", size, current_size);
            return ptr;
        }
    } else {
        block_t *block = (block_t *)meta;
        current_size = block->size;
        if (size <= current_size) {
            DBG("realloc size %zu fits in large block size %zu, returning same pointer", size, current_size);
            return ptr;
        }
    }
    void *new_ptr = malloc(size);
    if(new_ptr == NULL){
        DBG("realloc failed to allocate new block for size=%zu", size);
        
        return NULL;
    }
    size_t copy_size = (size < current_size) ? size : current_size;
    ft_memcpy(new_ptr, ptr, copy_size);
    DBG("realloc new block %p for size=%zu, old ptr=%p", new_ptr, size, ptr);
    free(ptr);
    
    return new_ptr;
}
