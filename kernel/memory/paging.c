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

page_directory_t *current_pd;

/**
 * Constructs new paging structures to allow for 4KiB page sizes
 * and remaps kernel to the same location
 */
void paging_init() {
    // Install page fault handler
    isr_install_handler(14, _page_fault_handler);

    current_pd = (page_directory_t *)kvalloc(sizeof(page_directory_t));
    page_table_t *pt = (page_table_t *)kvalloc(sizeof(page_table_t));

    current_pd->phys = ((uint32_t) current_pd->table_phys) - VIRTUAL_BASE;

    // Set all page directory entries to not present, read/write, supervisor
    uint32_t i;
    for (i=0; i < 1024; i++) {
        current_pd->table_phys[i] = PD_RW;
    }

    // Add page table containing kernel (#786)
    uint32_t kernel_table = VIRTUAL_BASE / 0x400000;

    current_pd->table[kernel_table] = pt;
    current_pd->table_phys[kernel_table] |= ((uint32_t)pt - VIRTUAL_BASE) | PD_PRESENT;

    // Map all paging structures to last 4MiB of memory
    current_pd->table_phys[1023] |= (uint32_t)current_pd->phys | PD_PRESENT;

    // Map from 0x0000 to the end of the kernel heap
    for (i=0; i<meminfo.kernel_heap_end - VIRTUAL_BASE; i += 0x1000) {
        current_pd->table[kernel_table]->page_phys[i / 0x1000] = i | PT_RW | PT_PRESENT;
        bitmap_set(mem_bitmap, i / 0x1000);
    }

    meminfo.kernel_heap_brk = i + VIRTUAL_BASE;

    // Load new page directory
    load_page_dir(current_pd->phys);
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
    printf("\nPAGE FAULT at 0x%x\nFlags:", faulting_address);
    if (present) printf(" [NOT PRESENT]");
    if (rw) printf(" [NOT WRITABLE]");
    if (us) printf(" [USERMODE]");
    if (reserved) printf(" [RESERVED]");
    abort();
}

uint32_t get_phys(void *virt) {
    uint32_t itable = (uint32_t)virt >> 22;
    uint32_t ipage = (uint32_t)virt >> 12 & 0x03FF;
 
    uint32_t *pd = (uint32_t *)0xFFFFF000; 
    uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * itable);

    // Check presence of PDE
    if (!(pd[itable] & PD_PRESENT))
        return -1;

    // Check presence of PTE
    if (!(pt[ipage] & PT_PRESENT))
        return -1;
 
    return ((pt[ipage] & ~0xFFF) + ((uint32_t)virt & 0xFFF));
}

// Simply maps virtual to physical
uint32_t map_page_to_phys(uint32_t virt, uint32_t phys, uint32_t pt_flags) {
    uint32_t itable = virt >> 22;
    uint32_t ipage = virt >> 12 & 0x03FF;

    // Page directory entry not present
    if (!(current_pd->table_phys[itable] & PD_PRESENT)) {
        // Create a new page table
        page_table_t *new_pt = (page_table_t *)kvalloc(sizeof(page_table_t));

        // Add table to directory
        current_pd->table_phys[itable] = get_phys(new_pt) | PD_PRESENT  | PD_RW;
        current_pd->table[itable] = new_pt;
    }
 
    // Add page to corresponding the new page table
    current_pd->table[itable]->page_phys[ipage] = phys | (pt_flags & 0xFFF) | PT_PRESENT;

    //printf("Map page 0x%x to phys 0x%x\n", virt, phys);

    return virt;
}

/**
 * Map virtual address to first available physical page
 */
uint32_t map_page(uint32_t virt, uint32_t pt_flags) {
    return map_page_to_phys(virt, mem_allocate_frame() * 0x1000, pt_flags);
}

void unmap_page(uint32_t virt) {
    uint32_t itable = virt >> 22;
    uint32_t ipage = virt >> 12 & 0x03FF;

    if (current_pd->table_phys[itable] & PD_PRESENT) {
        current_pd->table[itable]->page_phys[ipage] &= !PT_PRESENT;
        __flush_tlb_single(virt);
    }
}