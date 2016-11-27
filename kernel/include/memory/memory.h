#ifndef __MEMORY_MEMORY_H
#define __MEMORY_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#include <memory/multiboot.h>
#include <memory/paging.h>
#include <memory/heap.h>


#define PAGE_SIZE 4096

#define VIRTUAL_BASE 0xC0000000

/**
 * Enum declaring memory state values
 */
enum mem_states_t {
    MEM_RESERVED,
    MEM_FREE,
    MEM_NONEXISTANT
};

/**
 * Bitset containing the status of all frame numbers
 */
extern uint32_t *mem_bitmap;

/**
 * Structure containing internal data for i386 memory functions
 */
struct i386_mem_info {
    multiboot_info_t *mbi;
    uint32_t mmap_length;
    multiboot_memory_map_t *mmap;
    multiboot_elf_section_header_table_t *elf_sec;
    uint32_t kernel_reserved_start;
    uint32_t kernel_reserved_end;
    uint32_t multiboot_reserved_start;
    uint32_t multiboot_reserved_end;
    uint32_t kernel_heap_start;
    uint32_t kernel_heap_end;
    uint32_t kernel_heap_brk;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t highest_free_address;
};

extern struct i386_mem_info meminfo;

void mem_init();
void mem_init_bitmap();
uint32_t mem_allocate_frame();
void mem_free_frame(uint32_t frame);

void elf_sections_read();
void mem_print_reserved();
uint8_t mem_check_reserved(uint32_t addr);
uint8_t mmap_check_reserved(uint32_t addr);

#endif