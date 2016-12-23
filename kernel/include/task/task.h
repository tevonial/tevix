#ifndef __TASK_TASK_H
#define __TASK_TASK_H

#include <stdint.h>
#include <stdbool.h>

#include <memory/multiboot.h>
#include <memory/paging.h>
#include <memory/heap.h>


#define USER_CS    0x1B
#define USER_DS	   0x23
#define USER_STACK 0xBFFFFFFC

typedef struct task {
	uint32_t pid;
	
	uint32_t ebp;
	uint32_t esp;
	uint32_t eip;

	uint8_t ring;
	uint32_t esp0;

	page_directory_t *pd;
	struct task *next;

	bool fresh;
} task_t;

uint32_t pids;
task_t *todo;
task_t *task;


// thread.c
extern uint32_t fork();
extern void exec(char *name);
extern void switch_task();
extern void switch_to_user_mode();
extern void syscall_init();

// context.asm
extern uint32_t get_eip();
extern uint32_t get_boot_stack();
extern void switch_context(uint32_t eip, uint32_t esp, uint32_t ebp, uint32_t pd);
extern void become_user(uint32_t ds, uint32_t esp, uint32_t cs, void *eip);

#endif