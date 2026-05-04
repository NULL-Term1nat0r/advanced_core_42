#include "malloc.h"

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