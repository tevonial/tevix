#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <memory/bitmap.h>
#include <memory/memory.h>
#include <memory/multiboot.h>
#include <memory/paging.h>
#include <core/interrupt.h>

extern void load_page_dir(uint32_t *);
extern void disable_pse();
extern uint32_t get_faulting_address();

page_directory_t page_directory;

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

    page_directory.phys = (uint32_t)pdt - VIRTUAL_BASE;

    // Wipe page directory
    uint32_t i;
    for (i=0; i < 1024; i++) {
        // Set all page directory entries to not present, read/write, supervisor
        pdt[i] = PD_RW;
        page_directory.present[i] = false;
    }

    // Add page table containing kernel (#786)
    uint32_t table_index = VIRTUAL_BASE / 0x400000;
    pdt[table_index] |= ((uint32_t)pt - VIRTUAL_BASE) | PD_PRESENT;
    page_directory.present[table_index] = true;

    // Map all paging structures to last 4MiB of memory
    pdt[1023] |= (uint32_t)page_directory.phys | PD_PRESENT;
    page_directory.present[1023] = true;

    // Map from 0x0000 to the end of the kernel heap
    for (i=0; i<meminfo.kernel_heap_curpos - VIRTUAL_BASE; i += 0x1000) {
        pt[i / 0x1000] = i | PT_RW | PT_PRESENT;
        bitmap_set(mem_bitmap, i / 0x1000);
    }

    meminfo.kernel_brk = i + VIRTUAL_BASE;

    // Load new page directory
    load_page_dir((uint32_t)page_directory.phys);
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

uint32_t get_phys(void *virtualaddr)
{
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;
 
    unsigned long * pd = (unsigned long *)0xFFFFF000;
    // Here you need to check whether the PD entry is present.
 
    unsigned long * pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.
 
    return ((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

uint32_t map_page_to_phys(uint32_t virt, uint32_t phys, uint32_t pt_flags) {
    uint32_t pdindex = virt >> 22;
    uint32_t ptindex = virt >> 12 & 0x03FF;
 
    uint32_t *pd = (uint32_t*)0xFFFFF000;

    // Page directory entry in not present
    if (!(pd[pdindex] & PD_PRESENT)) {
        int temp_index = 0;
        if (ptindex == 0)
            temp_index++;

        // Map a page for the new page table using temporary page table
        temp_pt[temp_index] = mem_allocate_frame() * 0x1000 | PT_PRESENT | PT_RW;
        pd[pdindex] = get_phys(temp_pt) | PD_PRESENT | PD_RW;

        // Map page table to its own first entry, unmap temp table
        uint32_t *new_pt = (uint32_t*)(pdindex << 22);
        new_pt[temp_index] = temp_pt[temp_index];
        pd[pdindex] = get_phys(new_pt) | PD_PRESENT  | PD_RW;
    }
 
    uint32_t *pt = ((uint32_t*)0xFFC00000) + (0x400 * pdindex);
 
    // Add page to corresponding page table
    pt[ptindex] = phys | (pt_flags & 0xFFF) | 0x01;

    printf("0x%x mapped to 0x%x\n", phys, virt);

    return virt;
}

uint32_t map_page(uint32_t virt, uint32_t pt_flags) {
    return map_page_to_phys(virt, mem_allocate_frame() * 0x1000, pt_flags);
}
