#ifndef __MEMORY_PAGING_H
#define __MEMORY_PAGING_H

#include <stdint.h>

#include <core/interrupt.h>

// Page Table Entry flags
#define PT_PRESENT (1<<0)      // Is the page present?
#define PT_RW (1<<1)           // Is the page read/write?
#define PT_USER (1<<2)         // Is the page user-mode?
#define PT_WRITETHROUGH (1<<3) // Is writethrough enabled for the page?

// Page Directory Entry flags
#define PD_PRESENT (1<<0)
#define PD_RW (1<<1)
#define PD_USER (1<<2)
#define PD_WRITETHROUGH (1<<3)

// Page Fault error flags
#define PF_PRESENT (1<<0)      // Was the page present?
#define PF_RW (1<<1)           // Was the page wrongfully written to?
#define PF_USER (1<<2)         // Was the CPU in user-mode?
#define PF_RESERVED (1<<3)     // Were the CPU-reserved bytes overwritten?
#define PF_ID (0x10)           // Was the fault caused by an instruction fetch?

/*typedef struct {
	uint32_t *phys;
	bool present[1024];
	//page_table_t *pt[1024];
} page_directory_t;*/

void paging_init();
uint32_t map_page_to_phys(uint32_t virt, uint32_t phys, uint32_t pt_flags);
uint32_t map_page(uint32_t virt, uint32_t pt_flags);
uint32_t get_phys(void *virt);
bool is_page_mapped(void *addr);

void _page_fault_handler(struct regs *r);

static inline void __flush_tlb_single(unsigned long addr) {
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline void invlpg(void* m) {
    /* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
    asm volatile ("invlpg (%0)" : : "b"(m) : "memory");
}

#endif