#include "malloc.h"

void free(void *pointer){
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