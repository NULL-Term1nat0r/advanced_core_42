#ifndef MALLOC_H
# define MALLOC_H

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <valgrind/memcheck.h>
#include "../libft/libft.h"


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

extern heap_t heap;

#define TINY_MAX   64
#define SMALL_MAX  1024
#define BLOCKS_PER_ZONE 10

#define TINY_ZONE_SIZE  (BLOCKS_PER_ZONE * (TINY_MAX + sizeof(block_t)))
#define SMALL_ZONE_SIZE (BLOCKS_PER_ZONE * (SMALL_MAX + sizeof(block_t)))

#define DEBUG 0

#if DEBUG
# define DBG(fmt, ...) \
    printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
# define DBG(fmt, ...)
#endif


#define USE_VALGRIND 0

#if USE_VALGRIND

# define VG_MALLOC(addr, size) \
    VALGRIND_MALLOCLIKE_BLOCK(addr, size, 0, 0)

# define VG_FREE(addr) \
    VALGRIND_FREELIKE_BLOCK(addr, 0)

#else

# define VG_MALLOC(addr, size)
# define VG_FREE(addr)

#endif

void show_alloc_mem();
void free(void *pointer);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

//utils
block_t *get_last_block(block_t *head);
size_t align_page(size_t size);
block_t *find_free_block(block_t *head);
block_t *init_zone_blocks(size_t zone_size, size_t block_size, zone_type_t type);
zone_t *create_zone(zone_type_t type, size_t block_size);
zone_t *find_zone(zone_type_t type);
int setup_zones(void);

#endif
