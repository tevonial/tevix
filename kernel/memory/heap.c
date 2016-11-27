#include <stdint.h>

#include <memory/memory.h>
#include <memory/paging.h>
#include <memory/heap.h>


void heap_init() {
    heap_list.limit = MAX_HEAP_BLOCKS;
    heap_list.size = 0;
}

heap_block_t *heap_list_add(void *addr, uint32_t size) {
    uint32_t i = 0;
    while (block_pool[i].addr != (void *)0)
        i++;

    heap_block_t *new_block = &block_pool[i];

    new_block->addr = addr;
    new_block->size = size;
    new_block->free = true;

    // Insert entry in sorted list
    i = 0;
    while (i < heap_list.size) {
        if (heap_list.block[i++]->size >= new_block->size)
            break;
    }

    heap_block_t *tmp, *tmp2;

    tmp = heap_list.block[i];
    heap_list.block[i] = new_block;

    while (i < heap_list.size) {
        i++;
        tmp2 = heap_list.block[i];
        heap_list.block[i] = tmp;
        tmp = tmp2;
    }

    heap_list.size++;

    return new_block;
}

void heap_list_remove(heap_block_t *block) {
    block->addr = (void *)0;
    heap_list.size--;

    uint32_t i = 0;
    while (heap_list.block[i] != block)
        i++;

    while (i < heap_list.size) {
        heap_list.block[i] = heap_list.block[i+1];
        i++;
    }
}

void kfree(void *addr) {
    heap_block_t *left, *block, *right;
    left = right = heap_list.block[0];

    // Find indexes of block, block to left, and block to right
    for (uint32_t i=0; i < heap_list.size; i++) {
        if (heap_list.block[i]->addr == addr)
            block = heap_list.block[i];

        if (heap_list.block[i]->addr < addr &&
            heap_list.block[i]->addr > left->addr)
            left = heap_list.block[i];

        if (heap_list.block[i]->addr > addr &&
            heap_list.block[i]->addr < right->addr)
            right = heap_list.block[i];
    }

    printf("kfree 0x%x - 0x%x\n", block->addr, block->addr + block->size);

    // Prepare to create new block
    void *new_addr = block->addr;
    uint32_t new_size = block->size;

    // Check if block to left is free
    if (left->addr + left->size == block->addr && left->free) {
        new_addr = left->addr;
        new_size += left->size;

        heap_list_remove(left);
    }

    // Check if block to right is free
    if (block->addr + block->size == right->addr && right->free) {
        new_size += right->size;

        heap_list_remove(right);
    }

    if (new_addr) {
        heap_list_remove(block);
        // Create new, bigger block
        heap_list_add(new_addr, new_size);
    } else {
        block->free = true;
    }
}

void *kmalloc(uint32_t size) {
    heap_block_t *block;

    for (int i=0; i<heap_list.size; i++) {
        block = heap_list.block[i];
        if (block->size >= size && block->free == true) {

            block->free = false;        // Reserve block
            if (block->size > size) {   // Split block if neccesary
                heap_list_add((void *)((uint32_t)block->addr + size), block->size - size);
                block->size = size;
            }
            printf("alloc 0x%x - 0x%x\n", block->addr, block->addr + block->size);
            return block->addr;
        }
    }

    // No block found, create new block
    block = heap_list_add((void *)meminfo.kernel_heap_end, size);
    block->free = false;

    printf("alloc 0x%x - 0x%x\n", block->addr, block->addr + block->size);

    while (meminfo.kernel_heap_brk < (uint32_t)block->addr + size) {
        map_page(meminfo.kernel_heap_brk, PT_RW);
        meminfo.kernel_heap_brk += 0x1000;
    }
    meminfo.kernel_heap_end += size;

    return block->addr;
}

void *kvalloc(uint32_t size) {
    uint32_t addr = meminfo.kernel_heap_end;

    // Page align start address
    if (addr % PAGE_SIZE != 0) {
        addr &= 0x100000000 - PAGE_SIZE;
        addr += PAGE_SIZE;

        // Add gap created from alignment to heap blocks
        heap_list_add((void *)meminfo.kernel_heap_end, addr - meminfo.kernel_heap_end);
    }

    // Increase kernel heap size if needed
    while (addr + size > meminfo.kernel_heap_brk) {
        map_page(meminfo.kernel_heap_brk, PT_RW);
        meminfo.kernel_heap_brk += 0x1000;
    }

    // Increase the heap pointer
    meminfo.kernel_heap_end = addr;
    meminfo.kernel_heap_end += size;

    // Allocate block in heap
    heap_block_t *new_block = heap_list_add((void *)addr, size);
    new_block->free = false;

    printf("alloc 0x%x - 0x%x\n", addr, addr + size);

    return (void *)addr;
}