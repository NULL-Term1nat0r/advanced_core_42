#include "malloc_bonus.h"

void lst_add_back(zone_t **head, zone_t *new_lst){
    if (head == NULL || new_lst == NULL) {
        DBG("lst_add_back invalid args head=%p new_lst=%p", (void *)head, (void *)new_lst);
        return;
    }
    new_lst->next = NULL;
    if (*head == NULL) {
        DBG("lst_add_back creating new head %p", new_lst);
        new_lst->prev = NULL;
        *head = new_lst;
        return;
    }
    zone_t *last = *head;
    while (last->next)
        last = last->next;
    DBG("lst_add_back appending %p after %p", new_lst, last);
    last->next = new_lst;
    new_lst->prev = last;
}

void block_add_back(block_t **head, block_t *new_block){
    if (head == NULL || new_block == NULL) {
        DBG("block_add_back invalid args head=%p new_block=%p", (void *)head, (void *)new_block);
        return;
    }
    new_block->next = NULL;
    if (*head == NULL) {
        DBG("block_add_back creating new head %p", new_block);
        new_block->prev = NULL;
        *head = new_block;
        return;
    }
    block_t *last = *head;
    while (last->next)
        last = last->next;
    DBG("block_add_back appending %p after %p", new_block, last);
    last->next = new_block;
    new_block->prev = last;
}

void lst_detach(zone_t **head, zone_t *lst){
    if (!head || !lst) {
        DBG("lst_detach invalid args head=%p lst=%p", (void *)head, (void *)lst);
        return;
    }
    if (lst->prev)
        lst->prev->next = lst->next;
    else
        *head = lst->next;
    if (lst->next)
        lst->next->prev = lst->prev;
    lst->next = NULL;
    lst->prev = NULL;
    DBG("lst_detach removed %p from list head now %p", lst, (void *)*head);
}

void block_detach(block_t **head, block_t *block){
    if (!head || !block) {
        DBG("block_detach invalid args head=%p block=%p", (void *)head, (void *)block);
        return;
    }
    if (block->prev)
        block->prev->next = block->next;
    else
        *head = block->next;
    if (block->next)
        block->next->prev = block->prev;
    block->next = NULL;
    block->prev = NULL;
    DBG("block_detach removed %p from list head now %p", block, (void *)*head);
}