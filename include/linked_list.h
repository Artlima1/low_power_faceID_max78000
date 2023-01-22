#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

#include <stdint.h>

void * list_create();
uint8_t list_insert(void * head, uint16_t value);
uint8_t list_insert_array(void * head, uint16_t * values, uint32_t size);
void list_free(void * head);
uint8_t list_get(void * head, uint32_t index, uint16_t ** value);
uint8_t list_get_next(uint16_t ** value_pointer);

#endif // __LINKED_LIST_H__