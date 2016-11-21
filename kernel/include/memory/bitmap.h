#ifndef __MEMORY_BITMAP_H
#define __MEMORY_BITMAP_H

#include <stdint.h>

#define INDEX_TO_BIT(a) (a / 32)
#define OFFSET_TO_BIT(a) (a % 32)

void bitmap_set(uint32_t *bitmap, uint32_t n);
uint32_t bitmap_get(uint32_t *bitmap, uint32_t n);
void bitmap_clear(uint32_t *bitmap, uint32_t n);

#endif