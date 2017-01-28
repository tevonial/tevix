#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <memory/bitmap.h>
#include <memory/memory.h>
#include <memory/multiboot.h>
#include <memory/paging.h>
#include <core/interrupt.h>
#include <task/scheduler.h>

// Global pointer to page dir currently in use
page_directory_t *current_pd;

// Page tables are reserved in advance to avoid having no available mapped heap space
static page_table_t *reserved_pt;

// For copying pages
static void *tmp_src_page;
static void *tmp_dst_page;


/**
 * Constructs new paging structures to allow for 4KiB page sizes
 * and remaps kernel to the same virtual location
 */
void paging_init() {
    // Fault handler will provide useful debugging info
    isr_install_handler(14, page_fault_handler);

    // Allocate the initial PDT
    page_directory_t *pd = (page_directory_t *)kvalloc(sizeof(page_directory_t));
    pd->phys = ((uint32_t) pd->table_phys) - VIRTUAL_BASE;
    memset(pd->table, 0, 1024);
    memset(pd->table_phys, 0, 1024);

    // Rotating pointer to reserved page table
    reserved_pt = (page_table_t *)kvalloc(sizeof(page_table_t));
    memset(reserved_pt, 0, 1024);


    // Map kernel to higher half (0xC0000000 + kernel)
    uint32_t i, t;
    for (i=0; i<meminfo.kernel_heap_end - VIRTUAL_BASE; i += 0x1000) {
        // Create new page tables as needed
        if (!((i + VIRTUAL_BASE) % 0x400000)) {
            t = (i + VIRTUAL_BASE) / 0x400000;
            pd->table[t] = (page_table_t *)kvalloc(sizeof(page_table_t));
            pd->table_phys[t] = ((uint32_t)pd->table[t] - VIRTUAL_BASE) | PT_PRESENT | PT_RW;

        }

        // Reserve and add page
        pd->table[t]->page_phys[i / 0x1000] = i | PT_RW | PT_PRESENT;
        bitmap_set(mem_bitmap, i / 0x1000);
    }

    // Highest mapped address of heap area
    meminfo.kernel_heap_brk = i + VIRTUAL_BASE;

    // Load new page directory
    switch_pd(pd);
    // Enable 4KiB pages
    disable_pse();
}

void switch_pd(page_directory_t *pd) {
    current_pd = pd;
    load_page_dir(pd->phys);
}

void page_fault_handler(registers_t *r) {
    // Dump information about fault to screen
    printf("\nPAGE FAULT at 0x%x\nFlags:", get_faulting_address());
    if (!(r->err_code & PF_PRESENT)) printf(" [NONEXISTENT]");
    if (r->err_code & PF_RW)         printf(" [READONLY]");
    if (r->err_code & PF_USER)       printf(" [PRIVILEGE]");
    if (r->err_code & PF_RESERVED)   printf(" [RESERVED]");

    // Additional debugging info
    printf("\nebp: %x esp: %x eax: %x ecx: %x edx: %x\n", r->ebp, r->esp, r->eax, r->ecx, r->edx);
    printf("ss=%x useresp=%x eflags=%x cs=%x eip=%x\n", r->ss, r->useresp, r->eflags, r->cs, r->eip);
    abort();
}

uint32_t get_phys(void *virt) {
    uint32_t itable = (uint32_t)virt >> 22;
    uint32_t ipage = (uint32_t)virt >> 12 & 0x03FF;

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

        // Add reserved table to directory
        current_pd->table_phys[itable] = get_phys(reserved_pt) | PT_PRESENT  | (flags & 0xFFF);
        current_pd->table[itable] = reserved_pt;

        // Set aside memory for another page table for next time
        reserved_pt = (page_table_t *)kvalloc(sizeof(page_table_t));
    }
    
    // Add page to corresponding the new page table
    current_pd->table[itable]->page_phys[ipage] = phys | PT_PRESENT | (flags & 0xFFF);

    return virt;
}


// Map virtual address to first available physical page
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

    // New pointers
    uint32_t ebp = stack - ((uint32_t)_init_stack_end - init_ebp);
    uint32_t esp = stack - ((uint32_t)_init_stack_end - init_esp);

    uint32_t *old = (uint32_t *)init_esp;
    uint32_t *new = (uint32_t *)esp;

    for (uint32_t i=stack; i>= (stack-limit); i-=0x1000)
        map_page(i, PT_RW);

    for (uint32_t i=0; i<stack - esp; i++) {

        // Offset anything that COULD be a stack frame pointer
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

    // Reuse and recycle (the old stack)
    memset(_init_stack_start, 0, _init_stack_end - _init_stack_start);
    heap_list_add(_init_stack_start, _init_stack_end - _init_stack_start);
}

// Clone an entire VAS
page_directory_t *clone_pd(page_directory_t* src) {

    page_directory_t *new_pd = (page_directory_t *)kvalloc(sizeof(page_directory_t));

    // Set physical address of page directory
    new_pd->phys = get_phys((void *)(new_pd->table_phys));

    // Set aside space for pointers without wasting actual RAM
    if (!tmp_dst_page) {
        tmp_dst_page = (void *)kvalloc(PAGE_SIZE);
        tmp_src_page = (void *)kvalloc(PAGE_SIZE);
        mem_free_frame(get_phys(tmp_dst_page) / 0x1000);
        mem_free_frame(get_phys(tmp_src_page) / 0x1000);
    }

    for (int i=0; i<1024; i++) {
        if (!src->table[i]) {
            new_pd->table_phys[i] = 0;
            continue;
        }


        if (i >= (VIRTUAL_BASE / 0x400000) && i <= (meminfo.kernel_heap_brk / 0x400000)) {
            // If page includes kernel data, point to same page tables
            new_pd->table[i] = src->table[i];
            new_pd->table_phys[i] = src->table_phys[i];

        } else {
            // Or make a copy for user data
            if (src->table_phys[i] & PT_PRESENT) {
                new_pd->table[i] = copy_pt(src->table[i]);
                new_pd->table_phys[i] = get_phys(new_pd->table[i]) | (src->table_phys[i] & 0xFFF);
            }
        }
    }

    return new_pd;
}

// Copy a page table and each of its present pages
page_table_t *copy_pt(page_table_t *src) {

    page_table_t *new_pt = (page_table_t *)kvalloc(sizeof(page_table_t));

    for (uint32_t i=0; i<1024; i++) {

        // Copy physical page
        if (src->page_phys[i] & PT_PRESENT) {
            map_page(tmp_dst_page, PT_RW);
            map_page_to_phys(tmp_src_page, src->page_phys[i], PT_RW);
            invlpg(tmp_src_page);
            invlpg(tmp_dst_page);

            new_pt->page_phys[i] = get_phys(tmp_dst_page) | (src->page_phys[i] & 0xFFF);
            memcpy(tmp_dst_page, tmp_src_page, 0x1000);
        }
    }

    return new_pt;
}
