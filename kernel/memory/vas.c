#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <memory/memory.h>

extern uint32_t *get_stack_info();

page_directory_t *vas_create(page_directory_t* base) {
	page_directory_t *new_pd = (page_directory_t *)kvalloc(sizeof(page_directory_t));

	// Set physical address of page directory
	new_pd->phys = get_phys((void *)new_pd) + (new_pd->table_phys - (uint32_t *)new_pd);

	// Set aside some kernel space for temporary page mappings
	if (!vas_dst_page) {
		vas_dst_page = (void *)kvalloc(PAGE_SIZE);
		vas_src_page = (void *)kvalloc(PAGE_SIZE);
		mem_free_frame(get_phys(vas_dst_page) / 0x1000);
		mem_free_frame(get_phys(vas_src_page) / 0x1000);
	}

	uint32_t i, kernel_table = VIRTUAL_BASE / 0x400000;

	for (i=0; i<kernel_table; i++) {		// Copy  page tables
		if (base->table_phys[i] & PD_PRESENT) {
			new_pd->table[i] = vas_copy_pt(base->table[i]);
			new_pd->table_phys[i] = get_phys(new_pd->table[i]);
		}
	}

	for (i=kernel_table; i<1024; i++) {		// Use kernel's page tables
		new_pd->table[i] = base->table[i];
		new_pd->table_phys[i] = base->table_phys[i];
	}

	return new_pd;
}

page_table_t *vas_copy_pt(page_table_t *src) {
	page_table_t *new_pt = (page_table_t *)kvalloc(sizeof(page_table_t));

	for (uint32_t i=0; i<1024; i++) {
		if (src->page_phys[i] & PT_PRESENT) {
			unmap_page(vas_dst_page);
			map_page(vas_dst_page, PT_RW);
			map_page_to_phys(vas_src_page, src->page_phys[i], PT_RW);

			new_pt->page_phys[i] = get_phys(vas_dst_page) | PT_PRESENT | PT_RW | PT_USER;
			memcpy(vas_dst_page, vas_src_page, PAGE_SIZE);
		}
	}

	return new_pt;
}

stack_t *copy_stack(stack_t *src) {
	uint32_t size = src->top - src->bottom;
	uint32_t used = src->top - src->esp;

	stack_t *stack = (stack_t *)kmalloc(sizeof(stack_t));
	stack->bottom = (uint32_t)kmalloc(size);
	stack->top = stack->bottom + size;
	stack->esp = stack->top - used;

	uint32_t *src_cursor = (uint32_t *)src->esp;
	uint32_t *cursor = (uint32_t *)stack->esp;

	while (cursor < stack->top) {
		if (*src_cursor > src->bottom && *src_cursor < src->top) {
			*cursor = stack->top - (src->top - *src_cursor);
		} else {
			*cursor = *src_cursor;
		}

		cursor += 4; src_cursor += 4;
	}
}