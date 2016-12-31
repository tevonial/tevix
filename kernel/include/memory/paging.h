#ifndef __MEMORY_PAGING_H
#define __MEMORY_PAGING_H

#include <stdint.h>
#include <stdbool.h>

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

typedef struct {
	uint32_t page_phys[1024];
} page_table_t;

typedef struct {
	page_table_t *table[1024];
	uint32_t table_phys[1024];
	uint32_t phys;
} page_directory_t;

extern page_directory_t *current_pd;
extern page_directory_t *kernel_pd;

extern void load_page_dir(uint32_t);
extern void disable_pse();
extern uint32_t get_faulting_address();

extern void paging_init();
extern void switch_pd(page_directory_t * pd);
extern uint32_t map_page_to_phys(uint32_t virt, uint32_t phys, uint32_t flags);
extern uint32_t map_page(uint32_t virt, uint32_t flags);
extern uint32_t get_phys(void *virt);
extern void move_stack(uint32_t stack, uint32_t limit);
extern page_directory_t *clone_pd(page_directory_t* base);
extern page_table_t *copy_pt(page_table_t *src);
extern void page_fault_handler(registers_t *r);

inline bool is_page_mapped(void *addr) {
    return (get_phys(addr) != -1);
}

static inline void invlpg(void* m) {
    /* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
    asm volatile ("invlpg (%0)" : : "b"(m) : "memory");
}

#endif