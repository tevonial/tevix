#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <memory/bitmap.h>
#include <memory/memory.h>


// Multiboot data
multiboot_info_t *mbi;
uint32_t mboot_magic;

// Bitmap containing reserved frames
uint32_t *mem_bitmap;

// Information about memory for kernel use
struct i386_mem_info meminfo;

void _load_mbi(uint32_t _mboot_magic, multiboot_info_t *_mbi) {
    mbi = _mbi;
    mboot_magic = _mboot_magic;
}

/**
 * Read multiboot information and initialize memory frame bitmap.
 * Initialize physical and virtual memory managers
 */
void mem_init() {
    mbi = (uint32_t)mbi + VIRTUAL_BASE;
    printf("mbi location: 0x%x\n", mbi);

    // Ensure multiboot has supplied required information
    if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC ||
        (mbi->flags & (1<<6)) == 0 ||   // Bit 6 signifies presence of mmap
        (mbi->flags & (1<<5)) == 0) {   // Bit 5 signifies presence of ELF info

        printf("Invalid multiboot structure or configuration!\n");
        abort();
    }

    // Fill local data structure with verified information
    meminfo.mbi = mbi;
    meminfo.mmap_length = mbi->mmap_length;
    meminfo.mmap = (multiboot_memory_map_t *) (mbi->mmap_addr + VIRTUAL_BASE);
    meminfo.elf_sec = &(mbi->u.elf_sec);
    meminfo.multiboot_reserved_start = (uint32_t)mbi;
    meminfo.multiboot_reserved_end = (uint32_t)(mbi + sizeof(multiboot_info_t));
    meminfo.mem_upper = mbi->mem_upper;
    meminfo.mem_lower = mbi->mem_lower;

    if (mbi->flags & (1<<3)) {
        multiboot_module_t *mod = (multiboot_module_t *) (mbi->mods_addr + VIRTUAL_BASE);
        meminfo.initrd_start = mod->mod_start + VIRTUAL_BASE;
        meminfo.initrd_end = mod->mod_end + VIRTUAL_BASE;
        printf("Module loaded to 0x%x - 0x%x\n", meminfo.initrd_start, meminfo.initrd_end);
    } else {
        printf("No module loaded\n");
    }

    // Parse ELF sections
    elf_sections_read();

    // Find the highest free address
    meminfo.highest_free_address = meminfo.mem_upper * 1024;

    // Start the kernel heap on the first page-aligned address after the kernel
    meminfo.kernel_heap_start = (meminfo.initrd_end + 0x1000) & 0xFFFFF000;
    meminfo.kernel_heap_end = meminfo.kernel_heap_start;

    //Initialize brk to 4MiB past virtual base address
    meminfo.kernel_heap_brk = VIRTUAL_BASE + 0x400000;

    // Initialize heap allocator
    heap_init();

    // Allocate required memory for mem frame bitset and mark reserved frames
    uint32_t bitmap_size = meminfo.highest_free_address / (0x1000 * 32);
    if (meminfo.highest_free_address % (0x1000 * 32) != 0)
        bitmap_size++;

    // Allocate bitmap
    mem_bitmap = (uint32_t *)kvalloc(bitmap_size * sizeof(uint32_t));
    memset((void *)mem_bitmap, 0, meminfo.highest_free_address/PAGE_SIZE);
    mem_init_bitmap();
}

/**
 * Initialize the bitmap containing reserved frames
 */
void mem_init_bitmap() {
    uint32_t i;

    // Iterate through memory and mark reserved frames in bitmap
    for (i = 0x0000; i + PAGE_SIZE < meminfo.highest_free_address; i += PAGE_SIZE) {
        // Check if the current address is reserved
        if (mem_check_reserved(i) == MEM_RESERVED) {
            // Mark frame as reserved in the bitmap
            bitmap_set(mem_bitmap, i / PAGE_SIZE);
        }
    }
}

/**
 * Allocates a frame and returns it's index
 * @return index of allocated frame
 */
uint32_t mem_allocate_frame() {
    uint32_t i, j;

    // Loop through bitmap until a free frame is found
    for (i=0; i < INDEX_TO_BIT(meminfo.highest_free_address/PAGE_SIZE); i++) {
        // Skip this entry if it is full
        if (mem_bitmap[i] == 0xFFFFFFFF) continue;

        // Check if this entry has a free frame
        for (j=0; j < 32; j++) {
            if (!(mem_bitmap[i] & (1 << j))) {
                // This frame is free, return it
                bitmap_set(mem_bitmap, i * 32 + j);
                return i * 32 + j;
            }
        }
    }

    // No free frames were found
    return 0;
}

/**
 * Frees a frame
 * @param frame index of frame to free
 */
inline void mem_free_frame(uint32_t frame) {
    bitmap_clear(mem_bitmap, frame);
}


/**
 * Checks if the frame at memory address `addr` is in any reserved memory
 * @return true if memory is reserved, otherwise false
 */
uint8_t mem_check_reserved(uint32_t addr) {
    // End of the page that starts at addr
    uint32_t addr_end = addr + PAGE_SIZE;

    // Below 0x1000 is always reserved
    if (addr_end <= 0x1000) return MEM_RESERVED;

    // Check that memory doesn't overlap with kernel binary
    if ((addr >= meminfo.kernel_reserved_start && addr <= meminfo.kernel_reserved_end) ||
        (addr_end >= meminfo.kernel_reserved_start && addr_end <= meminfo.kernel_reserved_end)) {
        return MEM_RESERVED;
    }

    // Check that memory doesn't overlap with multiboot information structure
    if ((addr >= meminfo.multiboot_reserved_start && addr <= meminfo.multiboot_reserved_end) ||
        (addr_end >= meminfo.multiboot_reserved_start && addr_end <=meminfo.multiboot_reserved_end)) {
        return MEM_RESERVED;
    }

    // Check that the memory isn't marked as reserved in the memory map
    if (mmap_check_reserved(addr) != MEM_FREE) {
        return MEM_RESERVED;
    }

    // If we got here, it means the memory is not reserved
    return MEM_FREE;
}

/**
 * Checks if the frame at a given address is reserved in the memory map
 * @param  addr address of frame to check
 * @return reserved status
 */
uint8_t mmap_check_reserved(uint32_t addr) {
    uintptr_t cur_mmap_addr = (uintptr_t)meminfo.mmap;
    uintptr_t mmap_end_addr = cur_mmap_addr + meminfo.mmap_length;

    // Loop through memory map entries until the one for the requested addr is found
    while (cur_mmap_addr < mmap_end_addr) {
        multiboot_memory_map_t *current_entry = (multiboot_memory_map_t *)cur_mmap_addr;

        // Check if requested addr falls within the bounds of this entry
        uint32_t current_entry_end = current_entry->addr + current_entry->len;

        if (addr >= current_entry->addr && addr <= current_entry_end) {
            //printf("[mem] 0x%x found after 0x%x\n", addr, current_entry->addr);
            // Check if the entry marks this address as reserved
            if (current_entry->type == MULTIBOOT_MEMORY_RESERVED) {
                return MEM_RESERVED; // Memory is reserved
            } else {
                // Check if a page would fit within this entry at this address
                // If it doesn't, it means that part of the page falls within
                // reserved memory.
                if (addr + PAGE_SIZE > current_entry_end) {
                    return MEM_RESERVED;
                } else {
                    return MEM_FREE; // Memory is not reserved
                }
            }
        }

        cur_mmap_addr += current_entry->size + sizeof(uintptr_t);
    }

    // If the memory address was not found in the multiboot memory map, return
    // false.
    return MEM_NONEXISTANT;
}

/**
 * Debug function to print out reserved memory regions to the screen
 */
void mem_print_reserved() {
    // Print out mem_lower and mem_upper
    printf("[mem] mem_lower: %u, mem_upper: %u, mem_total: %u\n",
            meminfo.mem_lower, meminfo.mem_upper, meminfo.mem_lower + meminfo.mem_upper);
    // Print out mmap
    uintptr_t cur_mmap_addr = (uintptr_t)meminfo.mmap;
    uintptr_t mmap_end_addr = cur_mmap_addr + meminfo.mmap_length;

    // Loop through memory map and print entries
    /*while (cur_mmap_addr < mmap_end_addr) {
        multiboot_memory_map_t *current_entry = (multiboot_memory_map_t *)cur_mmap_addr;

        printf("[mem] addr: 0x%lx len: 0x%lx reserved: %u\n", current_entry->addr,
                current_entry->len, current_entry->type);

        cur_mmap_addr += current_entry->size + sizeof(uintptr_t);
    }*/

    // Print out ELF
    printf("[mem] elf start: 0x%x\n[mem] elf end:   0x%x\n",
            meminfo.kernel_reserved_start, meminfo.kernel_reserved_end);

    // Print out mboot reserved
    //printf("[mem] mboot reserved start: 0x%x end: 0x%x\n",
    //        meminfo.multiboot_reserved_start, meminfo.multiboot_reserved_end);

    printf("[mem] kernel heap start: 0x%x\n", meminfo.kernel_heap_start);
    printf("[mem] kernel heap end:   0x%x\n", meminfo.kernel_heap_end);
    printf("[mem] kernel heap brk:   0x%x\n\n", meminfo.kernel_heap_brk);
}

/**
 * Read ELF32 section headers and determine kernel reserved memory
 */
void elf_sections_read() {
    // Get first section headers
    elf_section_header_t *cur_header = (elf_section_header_t *)(meminfo.elf_sec->addr + VIRTUAL_BASE);

    // Update local data to reflect reserved memory areas
    meminfo.kernel_reserved_start = (cur_header + 1)->sh_addr;
    meminfo.kernel_reserved_end = (cur_header + (meminfo.elf_sec->num) - 1)->sh_addr + VIRTUAL_BASE;
}
