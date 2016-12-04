#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <memory/bitmap.h>
#include <memory/memory.h>
#include <memory/multiboot.h>
#include <memory/paging.h>
#include <core/interrupt.h>

extern void load_page_dir(uint32_t);
extern void disable_pse();
extern uint32_t get_faulting_address();

uint32_t *temp_pt;

/**
 * Constructs new paging structures to allow for 4KiB page sizes
 * and remaps kernel to the same location
 */
void paging_init() {
    // Install page fault handler
    isr_install_handler(14, _page_fault_handler);

    uint32_t *pdt = (uint32_t *) kvalloc(sizeof(uint32_t) * 1024);
    uint32_t *pt = (uint32_t *) kvalloc(sizeof(uint32_t) * 1024);
    temp_pt = (uint32_t *) kvalloc(sizeof(uint32_t) * 1024);

    void *page_directory = (void *)pdt - VIRTUAL_BASE;

    // Set all page directory entries to not present, read/write, supervisor
    uint32_t i;
    for (i=0; i < 1024; i++) {
        pdt[i] = PD_RW;
    }

    // Add page table containing kernel (#786)
    uint32_t table_index = VIRTUAL_BASE / 0x400000;
    pdt[table_index] |= ((uint32_t)pt - VIRTUAL_BASE) | PD_PRESENT;

    // Map all paging structures to last 4MiB of memory
    pdt[1023] |= (uint32_t)page_directory | PD_PRESENT;

    // Map from 0x0000 to the end of the kernel heap
    for (i=0; i<meminfo.kernel_heap_end - VIRTUAL_BASE; i += 0x1000) {
        pt[i / 0x1000] = i | PT_RW | PT_PRESENT;
        bitmap_set(mem_bitmap, i / 0x1000);
    }

    meminfo.kernel_heap_brk = i + VIRTUAL_BASE;

    // Load new page directory
    load_page_dir(page_directory);
    // Enable 4KiB pages
    disable_pse();
}

void _page_fault_handler(struct regs *r) {
    // Get faulting address
    uint32_t faulting_address = get_faulting_address();

    // Read error flags
    bool present = !(r->err_code & PF_PRESENT);
    bool rw = (r->err_code & PF_RW);
    bool us = (r->err_code & PF_USER);
    bool reserved = (r->err_code & PF_RESERVED);

    // Dump information about fault to screen
    printf("\nPAGE FAULT at 0x%x\n   Flags:", faulting_address);
    if (present) printf(" [NOT PRESENT]");
    if (rw) printf(" [NOT WRITABLE]");
    if (us) printf(" [USERMODE]");
    if (reserved) printf(" [RESERVED]");
    abort();
}

uint32_t get_phys(void *virt)
{
    uint32_t pdindex = (uint32_t)virt >> 22;
    uint32_t ptindex = (uint32_t)virt >> 12 & 0x03FF;
 
    uint32_t *pd = (uint32_t *)0xFFFFF000; 
    uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * pdindex);

    // Check presence of PDE
    if (!(pd[pdindex] & PD_PRESENT))
        return -1;

    // Check presence of PTE
    if (!(pt[ptindex] & PT_PRESENT))
        return -1;
 
    return ((pt[ptindex] & ~0xFFF) + ((uint32_t)virt & 0xFFF));
}

// Simply maps virtual to physical
uint32_t map_page_to_phys(uint32_t virt, uint32_t phys, uint32_t pt_flags) {
    uint32_t pdindex = virt >> 22;
    uint32_t ptindex = virt >> 12 & 0x03FF;
 
    uint32_t *pd = (uint32_t*)0xFFFFF000;

    // Page directory entry not present
    if (!(pd[pdindex] & PD_PRESENT)) {
        int temp_index = 0;
        // Cannot map page table to the same location as the requested page
        if (ptindex == 0) temp_index++;

        // Map a page for the new page table using temporary page table
        temp_pt[temp_index] = mem_allocate_frame() * 0x1000 | PT_PRESENT | PT_RW;
        pd[pdindex] = get_phys(temp_pt) | PD_PRESENT | PD_RW;

        // Map page table to its own first entry, unmap temp table
        uint32_t *new_pt = (uint32_t*)(pdindex << 22);
        new_pt[temp_index] = temp_pt[temp_index];

        // Add table to directory
        pd[pdindex] = get_phys(new_pt) | PD_PRESENT  | PD_RW;
    }
 
    uint32_t *pt = ((uint32_t*)0xFFC00000) + (0x400 * pdindex);
 
    // Add page to corresponding page table
    pt[ptindex] = phys | (pt_flags & 0xFFF) | PT_PRESENT;

    //printf("Map page 0x%x to phys 0x%x\n", virt, phys);

    return virt;
}

/**
 * Map virtual address to first available physical page
 */
uint32_t map_page(uint32_t virt, uint32_t pt_flags) {
    return map_page_to_phys(virt, mem_allocate_frame() * 0x1000, pt_flags);
}