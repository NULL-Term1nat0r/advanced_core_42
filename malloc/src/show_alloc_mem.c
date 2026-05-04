#include "malloc.h"

void print_block_information(block_t *current_block){
    void *ret_pointer = (char *)current_block + sizeof(block_t);
    void *end_pointer = (char *)ret_pointer + current_block->size;
    printf("%p - %p : %li bytes\n", ret_pointer, end_pointer, current_block->size);
}

void print_zone_information(zone_t *current_zone, zone_type_t type){
    if(type == TINY_ZONE)
        printf("TINY: %p\n", current_zone->blocks);
    if(type == SMALL_ZONE)
        printf("SMALL: %p\n", current_zone->blocks);
    while(current_zone != NULL){
        if(current_zone->type == TINY_ZONE){
            block_t *current_block = current_zone->blocks;
            while(current_block != NULL){
                if(current_block->free == 0){
                    print_block_information(current_block);
                }
                current_block = current_block->next;
            }
        }
        current_zone = current_zone->next;
    }
}


void show_alloc_mem(){
    zone_t *current_zone = heap.zones;
    
    print_zone_information(current_zone, TINY_ZONE);
    print_zone_information(current_zone, SMALL_ZONE);
    block_t *current_block = heap.large_allocations;
    if(current_block != NULL)
        printf("LARGE: %p\n", current_block);
    while(current_block != NULL){
        block_t *last = get_last_block(current_block);
        print_block_information(current_block);
        current_block = current_block->next;
    }
}