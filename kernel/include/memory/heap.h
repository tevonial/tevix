#ifndef __MEMORY_HEAP_H
#define __MEMORY_HEAP_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_HEAP_BLOCKS 1000

typedef struct {
	void *addr;
	uint32_t size;
	bool free;
} heap_block_t;

typedef struct {
	uint32_t size;
	uint32_t limit;
	heap_block_t *block[MAX_HEAP_BLOCKS];
} heap_list_t;

heap_list_t heap_list;
heap_block_t block_pool[MAX_HEAP_BLOCKS];

void heap_init();
heap_block_t *heap_add(void *addr, uint32_t size);
void kfree(void *addr);
void *kmalloc(uint32_t size);
void *kvalloc(uint32_t size);
void sbrk();
void brk(void *addr);

#endif