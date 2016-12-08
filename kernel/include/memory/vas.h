#ifndef __MEMORY_VAS_H
#define __MEMORY_VAS_H

typedef struct {
	uint32_t bottom;
	uint32_t top;
	uint32_t esp;
} stack_t;

void *vas_src_page;
void *vas_dst_page;

page_table_t *vas_copy_pt(page_table_t *src);

#endif