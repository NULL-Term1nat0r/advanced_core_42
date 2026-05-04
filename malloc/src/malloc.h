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

static heap_t heap;

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