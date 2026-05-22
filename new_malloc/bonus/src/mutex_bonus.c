#include "malloc_bonus.h"

void lock_list(type_t type)
{
    if (type == TINY_BLOCK)
        pthread_mutex_lock(&global_heap.tiny_lock);
    else if (type == SMALL_BLOCK)
        pthread_mutex_lock(&global_heap.small_lock);
    else
        pthread_mutex_lock(&global_heap.large_lock);
}

void unlock_list(type_t type)
{
    if (type == TINY_BLOCK)
        pthread_mutex_unlock(&global_heap.tiny_lock);
    else if (type == SMALL_BLOCK)
        pthread_mutex_unlock(&global_heap.small_lock);
    else
        pthread_mutex_unlock(&global_heap.large_lock);
}
