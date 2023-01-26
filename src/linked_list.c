#include "linked_list.h"
#include <stdlib.h>

typedef struct node {
    uint16_t value;
    struct node * next;
} node_t;

void * list_create(){
    node_t * head = (node_t *) malloc(sizeof(node_t));
    head->next = NULL;
    return (void *) head;
}

void list_free(void * head){
    node_t * h = (node_t *) head;

    node_t * curr = h;
    node_t * next = h->next;
    while(next != NULL){
        free(curr);
        curr = next;
        next = curr->next;
    }

}

uint8_t list_insert(void * head, uint16_t value){
    node_t * h = (node_t *) head;

    if(h->next == NULL){
        h->value = value;
        return 1;
    }

    node_t * curr = h;
    node_t * next = h->next;
    while(next != NULL){
        curr = next;
        next = curr->next;
    }

    next = (node_t *) malloc(sizeof(node_t));
    if(next == NULL){
        return 0;
    }
    next->value = value;
    next->next = NULL;
    curr->next = next;
    return 1;
}

uint8_t list_insert_array(void * head, uint16_t * values, uint32_t size){
    node_t * h = (node_t *) head;

    node_t * curr = h;
    node_t * next = h->next;
    while(next != NULL){
        curr = next;
        next = curr->next;
    }

    int i;
    node_t * started = next;
    for (i=0; i<size; i++){
        next = (node_t *) malloc(sizeof(node_t));
        if(next == NULL){
            list_free((void *) started);
            return 0;
        }
        next->value = values[i];
        next->next = NULL;
        curr->next = next;

        curr = next;
    }

    return 1;
}

uint8_t list_get(void * head, uint32_t index, uint16_t ** value){
    node_t * h = (node_t *) head;

    int i;
    node_t * curr = h;
    for(i=0; i<index; i++){
        if(curr->next==NULL){
            return 0;
        }
        curr = curr->next;
    }
    *value = &curr->value;

    return 1;
} 

uint8_t list_get_next(uint16_t ** value_poiter){
    node_t * node = (node_t *) *value_poiter;
    if (node->next==NULL){
        return 0;
    }
    else {
        *value_poiter = &node->next->value;
    }
    return 1;
}