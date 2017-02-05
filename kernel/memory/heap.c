#include <stdint.h>

#include <memory/memory.h>
#include <memory/paging.h>
#include <memory/heap.h>


void heap_init() {
    heap_list.limit = MAX_HEAP_BLOCKS;
    heap_list.size = 0;
}

// Insert heap block in sorted list
static void heap_list_add(heap_block_t *block) {
    uint32_t i = 0;
    while (i < heap_list.size) {
        if (heap_list.block[i]->size >= block->size)
            break;
        i++;
    }

    heap_block_t *tmp, *tmp2;

    tmp = heap_list.block[i];
    heap_list.block[i] = block;

    while (i < heap_list.size) {
        i++;
        tmp2 = heap_list.block[i];
        heap_list.block[i] = tmp;
        tmp = tmp2;
    }

    heap_list.size++;
}

static void heap_list_remove(heap_block_t *block) {
    block->size = 0;
    heap_list.size--;

    uint32_t i = 0;
    while (heap_list.block[i] != block)
        i++;

    while (i < heap_list.size) {
        heap_list.block[i] = heap_list.block[i+1];
        i++;
    }
}

heap_block_t *heap_add(void *addr, uint32_t size) {
    uint32_t i = 0;
    while (block_pool[i].size != 0)
        i++;

    heap_block_t *new_block = &block_pool[i];

    new_block->addr = addr;
    new_block->size = size;
    new_block->free = true;

    heap_list_add(new_block);

    return new_block;
}

void kfree(void *addr) {
    heap_block_t *block;
    int i = 0;

    while (block = heap_list.block[i++])
        if (block->addr == addr)
            break;

    // Prepare to create new block
    void *new_addr = block->addr;
    uint32_t new_size = block->size;
    bool do_new_block = false;

    // Check if block to block->l is free
    if (block->l->free) {
        new_addr = block->l->addr;
        new_size += block->l->size;
        do_new_block = true;

        heap_list_remove(block->l);
    }

    // Check if block to block->r is free
    if (block->r->free) {
        new_size += block->r->size;
        do_new_block = true;

        heap_list_remove(block->r);
    }

    if (do_new_block) {
        heap_list_remove(block);
        // Create new, bigger block
        heap_add(new_addr, new_size);
    } else {
        block->free = true;
    }
}

void *kmalloc(uint32_t size) {
    heap_block_t *block;

    for (uint32_t i=0; i<heap_list.size; i++) {
        block = heap_list.block[i];
        if (block->size >= size && block->free == true) {

            block->free = false;        // Reserve block
            if (block->size > size) {   // Split block if neccesary
                heap_block_t *split = heap_add((void *)((uint32_t)block->addr + size), block->size - size);

                block->r = split;
                split->l = block;
                split->r = block->r;
                
                // Reorder block in list as size has changed
                heap_list_remove(block);
                block->size = size;
                heap_list_add(block);
            }
            return block->addr;
        }
    }

    // No block found, create new block
    block = heap_add((void *)meminfo.kernel_heap_end, size);
    block->free = false;

    block->l = heap_list.high;
    heap_list.high->r = block;
    heap_list.high = block;


    // Expand kernel heap as needed
    while (meminfo.kernel_heap_end + size > meminfo.kernel_heap_brk)
        sbrk();

    // Increase the heap pointer
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
        heap_block_t *gap = heap_add((void *)meminfo.kernel_heap_end, addr - meminfo.kernel_heap_end);

        gap->l = heap_list.high;
        heap_list.high->r = gap;
        heap_list.high = gap;
    }

    // Allocate block in heap
    heap_block_t *new_block = heap_add((void *)addr, size);
    new_block->free = false;

    new_block->l = heap_list.high;
    heap_list.high->r = new_block;
    heap_list.high = new_block;

    // Increase the heap pointer
    meminfo.kernel_heap_end = addr + size;

    // Expand kernel heap as needed
    while (meminfo.kernel_heap_end > meminfo.kernel_heap_brk)
        sbrk();

    return (void *)addr;
}

// Increase kernel heap size by one page increments
void sbrk() {
    map_page(meminfo.kernel_heap_brk, PT_RW);
    meminfo.kernel_heap_brk += 0x1000;
}

// Increase kernel heap to specified address
void brk(void *addr) {
    uint32_t i = meminfo.kernel_heap_brk;
    while (i < (uint32_t)addr) {
        if (!is_page_mapped((void *)i)) {
            map_page(i, PT_RW);
            i += 0x1000;
        }
    }
    meminfo.kernel_heap_brk = (uint32_t)addr;
}