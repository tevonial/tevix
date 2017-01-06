#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <memory/bitmap.h>
#include <memory/memory.h>
#include <memory/multiboot.h>
#include <memory/paging.h>
#include <core/interrupt.h>

page_directory_t *current_pd;
page_directory_t *kernel_pd;
static void *tmp_src_page;
static void *tmp_dst_page;

/**
 * Constructs new paging structures to allow for 4KiB page sizes
 * and remaps kernel to the same location
 */
void paging_init() {
    // Install page fault handler
    isr_install_handler(14, page_fault_handler);

    kernel_pd = (page_directory_t *)kvalloc(sizeof(page_directory_t));
    page_table_t *pt = (page_table_t *)kvalloc(sizeof(page_table_t));

    kernel_pd->phys = ((uint32_t) kernel_pd->table_phys) - VIRTUAL_BASE;

    // Set all page directory entries to not present, read/write, supervisor
    uint32_t i;
    for (i=0; i < 1024; i++) {
        kernel_pd->table_phys[i] = PT_RW;
    }

    // Add page table containing kernel (#786)
    uint32_t kernel_table = VIRTUAL_BASE / 0x400000;

    kernel_pd->table[kernel_table] = pt;
    kernel_pd->table_phys[kernel_table] |= ((uint32_t)pt - VIRTUAL_BASE) | PT_PRESENT;

    // Map from 0x0000 to the end of the kernel heap
    for (i=0; i<meminfo.kernel_heap_end - VIRTUAL_BASE; i += 0x1000) {
        kernel_pd->table[kernel_table]->page_phys[i / 0x1000] = i | PT_RW | PT_PRESENT;
        bitmap_set(mem_bitmap, i / 0x1000);
    }

    meminfo.kernel_heap_brk = i + VIRTUAL_BASE;

    // Load new page directory
    switch_pd(kernel_pd);
    // Enable 4KiB pages
    disable_pse();

    // Create new VAS and leave kernel_pd where it is
    switch_pd(clone_pd(kernel_pd));
}

void switch_pd(page_directory_t *pd) {
    current_pd = pd;
    load_page_dir(pd->phys);
}

void page_fault_handler(registers_t *r) {
    // Read error flags
    bool present = !(r->err_code & PF_PRESENT);
    bool rw = (r->err_code & PF_RW);
    bool us = (r->err_code & PF_USER);
    bool reserved = (r->err_code & PF_RESERVED);

    // Dump information about fault to screen
    printf("\nPAGE FAULT at 0x%x\nFlags:", get_faulting_address());
    if (!(r->err_code & PF_PRESENT)) printf(" [NONEXISTENT]");
    if (r->err_code & PF_RW)         printf(" [READONLY]");
    if (r->err_code & PF_USER)       printf(" [PRIVILEGE]");
    if (r->err_code & PF_RESERVED)   printf(" [RESERVED]");

    printf("\nebp: %x esp: %x eax: %x ecx: %x edx: %x\n", r->ebp, r->esp, r->eax, r->ecx, r->edx);
    printf("ss=%x useresp=%x eflags=%x cs=%x eip=%x\n", r->ss, r->useresp, r->eflags, r->cs, r->eip);
    abort();
}

uint32_t get_phys(void *virt) {
    uint32_t itable = (uint32_t)virt >> 22;
    uint32_t ipage = (uint32_t)virt >> 12 & 0x03FF;
 
    /* If paging stuctures were mapped to last PDE
    uint32_t *pd = (uint32_t *)0xFFFFF000; 
    uint32_t *pt = ((uint32_t *)0xFFC00000) + (0x400 * itable);*/

    // Check presence of PDE
    if (!(current_pd->table_phys[itable] & PT_PRESENT))
        return -1;

    // Check presence of PTE
    if (!(current_pd->table[itable]->page_phys[ipage]))
        return -1;

    return ((current_pd->table[itable]->page_phys[ipage] & ~0xFFF) + ((uint32_t)virt & 0xFFF));
}

// Simply maps virtual to physical
uint32_t map_page_to_phys(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t itable = virt >> 22;
    uint32_t ipage = virt >> 12 & 0x03FF;

    // Page directory entry not present
    if (!(current_pd->table_phys[itable] & PT_PRESENT)) {
        // Create a new page table
        page_table_t *new_pt = (page_table_t *)kvalloc(sizeof(page_table_t));

        // Add table to directory
        current_pd->table_phys[itable] = get_phys(new_pt) | PT_PRESENT  | (flags & 0xFFF);
        current_pd->table[itable] = new_pt;
    }
 
    // Add page to corresponding the new page table
    current_pd->table[itable]->page_phys[ipage] = phys | PT_PRESENT | (flags & 0xFFF);

    //printf("Map page 0x%x to phys 0x%x\n", virt, phys);

    return virt;
}

/**
 * Map virtual address to first available physical page
 */
uint32_t map_page(uint32_t virt, uint32_t flags) {
    return map_page_to_phys(virt, mem_allocate_frame() * 0x1000, flags);
}


void unmap_page(uint32_t virt) {
    uint32_t itable = virt >> 22;
    uint32_t ipage = virt >> 12 & 0x03FF;

    if (current_pd->table_phys[itable] & PD_PRESENT) {
        // Deallocate physical frame
        mem_free_frame((uint32_t)current_pd->table[itable]->page_phys[ipage] / 0x1000);

        // Remove mapping to page table
        current_pd->table[itable]->page_phys[ipage] &= !PT_PRESENT;
        
        // Notify MMU
        invlpg((void *)virt);
    }
}

void move_stack(uint32_t stack, uint32_t limit) {
    extern void _init_stack_start();
    extern void _init_stack_end();

    uint32_t init_ebp, init_esp;
    asm volatile("mov %%esp, %0" : "=r" (init_esp));
    asm volatile("mov %%ebp, %0" : "=r" (init_ebp));

    uint32_t ebp = stack - ((uint32_t)_init_stack_end - init_ebp);
    uint32_t esp = stack - ((uint32_t)_init_stack_end - init_esp);

    uint32_t *old = (uint32_t *)init_esp;
    uint32_t *new = (uint32_t *)esp;

    for (uint32_t i=stack; i>= (stack-limit); i-=0x1000)
        map_page(i, PT_RW);

    for (uint32_t i=0; i<stack - esp; i++) {
        if (old[i] >= init_esp && old[i] <= _init_stack_end)
            new[i] = stack - ((uint32_t)_init_stack_end - old[i]);
        else
            new[i] = old[i];
    }
    
    // New stack location open for business
    asm volatile("cli");
    asm volatile("mov %0, %%esp;" : : "r" (esp));
    asm volatile("mov %0, %%ebp;" : : "r" (ebp));
    asm volatile("sti");

    // Clear and add old stack space to the heap
    memset(_init_stack_start, 0, _init_stack_end - _init_stack_start);
    heap_list_add(_init_stack_start, _init_stack_end - _init_stack_start);
}


page_directory_t *clone_pd(page_directory_t* src) {
    page_directory_t *new_pd = (page_directory_t *)kvalloc(sizeof(page_directory_t));

    // Set physical address of page directory
    new_pd->phys = get_phys((void *)(new_pd->table_phys));

    // Set aside some kernel space for temporary page mappings
    if (!tmp_dst_page) {
        tmp_dst_page = (void *)kvalloc(PAGE_SIZE);
        tmp_src_page = (void *)kvalloc(PAGE_SIZE);
        mem_free_frame(get_phys(tmp_dst_page) / 0x1000);
        mem_free_frame(get_phys(tmp_src_page) / 0x1000);
    }

    for (int i=0; i<1024; i++) {
        if (!src->table[i])
            continue;

        if (src->table[i] == kernel_pd->table[i]) {
            new_pd->table[i] = src->table[i];
            new_pd->table_phys[i] = src->table_phys[i];
        } else {
            if (src->table_phys[i] & PT_PRESENT) {
                new_pd->table[i] = copy_pt(src->table[i]);
                new_pd->table_phys[i] = get_phys(new_pd->table[i]) | PT_PRESENT | PT_RW | PT_USER;
            }
        }
    }

    return new_pd;
}


page_table_t *copy_pt(page_table_t *src) {
    page_table_t *new_pt = (page_table_t *)kvalloc(sizeof(page_table_t));

    for (uint32_t i=0; i<1024; i++) {

        // Copy physical page
        if (src->page_phys[i] & PT_PRESENT) {
            map_page(tmp_dst_page, PT_RW);
            map_page_to_phys(tmp_src_page, src->page_phys[i], PT_RW);
            invlpg(tmp_src_page);
            invlpg(tmp_dst_page);

            new_pt->page_phys[i] = get_phys(tmp_dst_page) | PT_PRESENT | PT_RW | PT_USER;
            memcpy(tmp_dst_page, tmp_src_page, 0x1000);
        }
    }

    return new_pt;
}
