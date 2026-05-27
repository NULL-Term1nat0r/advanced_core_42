#ifndef MALLOC_H
# define MALLOC_H

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <valgrind/memcheck.h>
#include "../../libft/libft.h"
#include "../../printf/ft_printf.h"

#define TINY_BLOCK_SIZE 64
#define SMALL_BLOCK_SIZE 1024
#define BLOCKS_PER_ZONE 100

#define DEBUG 1

#if DEBUG
# define DBG(fmt, ...) ft_printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
# define DBG(fmt, ...)
#endif

typedef enum {
    TINY_BLOCK,
    SMALL_BLOCK,
    LARGE_BLOCK
} type_t;

typedef struct zone {
    void *base;
    type_t type;
    int number_of_blocks;
    size_t zone_size;
    size_t *requested_sizes;
    unsigned char *bitmap;
    void *memory;
    int free_count;
    struct zone *next;
    struct zone *prev;
} zone_t;

#define ZONE_BLOCK_SIZE(z)   ((z)->type == TINY_BLOCK ? TINY_BLOCK_SIZE : SMALL_BLOCK_SIZE)

/* Pack zone pointer + block index into one word.
 * Zones are page-aligned so bits 11:0 are always 0.
 * bit 11     : is_zone flag (1 = zone block, 0 = raw large-block pointer)
 * bits 10:0  : block_number (0-2047)                                       */
#define META_PACK(zone, n)   ((void *)((uintptr_t)(zone) | (1u << 11) | (unsigned)(n)))
#define META_IS_ZONE(meta)   ((uintptr_t)(meta) & (1u << 11))
#define META_ZONE(meta)      ((zone_t *)((uintptr_t)(meta) & ~(uintptr_t)0xFFF))
#define META_BLOCK_NUM(meta) ((int)((uintptr_t)(meta) & 0x7FF))

typedef struct block{
    size_t size;
    type_t type;
    int free;
    void *memory;
    struct block *next;
    struct block *prev;
} block_t;

typedef struct heap {
    pthread_mutex_t tiny_lock;
    pthread_mutex_t small_lock;
    pthread_mutex_t large_lock;
    int initialized;
    zone_t *free_tiny_zones;
    zone_t *free_small_zones;
    zone_t *full_tiny_zones;
    zone_t *full_small_zones;
    block_t *free_large_blocks;
    block_t *full_large_blocks;

} heap_t;

extern heap_t global_heap;
extern pthread_mutex_t heap_mutex;

void *calculate_zone_block_address(zone_t *zone, int block_number);
void lst_add_back(zone_t **head, zone_t *new_lst);
void block_add_back(block_t **head, block_t *new_block);
void lst_detach(zone_t **head, zone_t *lst);
void block_detach(block_t **head, block_t *block);

void lock_list(type_t type);
void unlock_list(type_t type);

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void show_alloc_mem();
void show_alloc_mem_ex(void);

#endif
