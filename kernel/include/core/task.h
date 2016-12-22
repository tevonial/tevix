#ifndef __CORE_TASK_H
#define __CORE_TASK_H

#include <stdint.h>
#include <stdbool.h>

#include <memory/multiboot.h>
#include <memory/paging.h>
#include <memory/heap.h>


typedef struct task {
	uint32_t pid;
	page_directory_t *pd;
	
	uint32_t ebp;
	uint32_t esp;
	uint32_t eip;

	struct task *next;
} task_t;

typedef struct {
	uint32_t ebp;
	uint32_t esp;
} stack_t;

uint32_t pids;
task_t *todo;
task_t *task;

void *vas_src_page;
void *vas_dst_page;

uint32_t fork();
void switch_task();
page_directory_t *clone_pd(page_directory_t* base);
page_table_t *copy_pt(page_table_t *src);
void switch_to_user_mode();
void init_syscall();

extern uint32_t get_eip();
extern uint32_t get_boot_stack();
extern void switch_context(uint32_t eip, uint32_t esp, uint32_t ebp, uint32_t pd);
extern uint32_t asm_test();

#endif