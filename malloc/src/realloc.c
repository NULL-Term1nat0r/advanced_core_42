#include "malloc.h"

void *realloc(void *ptr, size_t size){
    if (!ptr)
        return malloc(size);
    block_t *block = (block_t *)((char *)ptr - sizeof(block_t));
    if (block->size >= size)
        return ptr;
    void *new_ptr = malloc(size);
    if (!new_ptr)        
        return NULL;
    ft_memcpy(new_ptr, ptr, block->size);
    free(ptr);
    return new_ptr;
}