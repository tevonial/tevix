#ifndef __MEMORY_HEAP_H
#define __MEMORY_HEAP_H

#include <stdint.h>

typedef struct {
	void *addr;
	uint32_t size;
	bool free;
} heap_block_t;

typedef struct {
	uint32_t size;
	uint32_t limit;
	heap_block_t *block[1000];
} heap_list_t;

heap_list_t heap_list;
heap_block_t free_blocks[1000];

void heap_init();

void kfree(void *addr);
void *kmalloc(uint32_t size);
void *kvalloc(uint32_t size);

#endif
