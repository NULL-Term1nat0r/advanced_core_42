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
#include "../libft/libft.h"

#define TINY_BLOCK_SIZE 64
#define SMALL_BLOCK_SIZE 1024
#define BLOCKS_PER_ZONE 100

#define NUM_BYTES ((100 + 7) / 8)

#define DEBUG 1

#if DEBUG
# define DBG(fmt, ...) do { \
    char _dbg_buf[512]; \
    int _dbg_n = snprintf(_dbg_buf, sizeof(_dbg_buf), "[DEBUG] " fmt "\n", ##__VA_ARGS__); \
    write(STDERR_FILENO, _dbg_buf, _dbg_n > 0 ? _dbg_n : 0); \
} while(0)
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
    size_t block_size;
    size_t requested_sizes[BLOCKS_PER_ZONE];
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

extern heap_t global_heap;


void *calculate_zone_block_address(zone_t *zone, int block_number);
void lst_add_back(zone_t **head, zone_t *new_lst);
void block_add_back(block_t **head, block_t *new_block);
void lst_detach(zone_t **head, zone_t *lst);
void block_detach(block_t **head, block_t *block);

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void show_alloc_mem();

#endif
