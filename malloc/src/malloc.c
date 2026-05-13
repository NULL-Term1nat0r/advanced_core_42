#include "malloc.h"

heap_t heap;

/* ---------------- MALLOC CORE ---------------- */

void *malloc(size_t size)
{
    DBG("malloc size=%zu", size);

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
            VG_MALLOC((char *)new_block + sizeof(block_t), size);
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
        new_zone->blocks->free = 0;
        new_zone->blocks->size = block_size;
        void *ret = (char *)new_zone->blocks + sizeof(block_t);
        VG_MALLOC(ret, size);
        return ret;

        // DBG("no free block found");
    }

    block->free = 0;
    block->size = size;

    void *ret = (char *)block + sizeof(block_t);

    DBG("allocated ptr=%p", ret);
    
    VG_MALLOC(ret, size);

    return ret;
}



int main(void)
{
    int number_char_pointers = 2;
    char *arr[number_char_pointers];

    for (int i = 0; i < number_char_pointers; i++)
    {
        arr[i] = malloc(60);

        if (!arr[i])
        {
            // printf("allocation failed at i=%d", i);
            return 1;
        }

        arr[i][0] = 't';
        arr[i][1] = 'e';
        arr[i][2] = 's';
        arr[i][3] = 't';
    }

    void *pointer1 = malloc(2000);
    void *pointer2 = malloc(2000);
    void *pointer3 = malloc(2000);
    void *pointer4 = malloc(2000);

    // show_alloc_mem();

    void * new_pointer1 = realloc(pointer1, 2000);

    // show_alloc_mem();

    return 0;
}