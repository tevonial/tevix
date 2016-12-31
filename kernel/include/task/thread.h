#ifndef __TASK_THREAD_H
#define __TASK_THREAD_H

#include <stdint.h>
#include <stdbool.h>

#include <memory/multiboot.h>
#include <memory/paging.h>
#include <memory/heap.h>

#define KSTACK      0xF0000000
#define KSTACK_LIM  0x4000

#define USER_CS     0x1B
#define USER_DS     0x23
#define USER_STACK  0xBFFFFFFC


typedef struct thread {	
	uint32_t ebp;
	uint32_t esp;
	uint32_t eax;
	uint32_t cr3;
	uint32_t eip;

	uint32_t pid;
	uint8_t ring;
	uint32_t esp0;
	page_directory_t *pd;

	struct thread *next;
} thread_t;

uint32_t pids;
thread_t *thread;


// thread.c
extern thread_t *thread_init();
extern thread_t *constuct_thread(void *start);
extern uint32_t fork();
extern void preempt(registers_t *regs);
extern void exec(char *name);

// context.asm
extern uint32_t get_eip();
extern void switch_context(thread_t *from, thread_t *to);
extern void become_user(uint32_t ds, uint32_t esp, uint32_t cs, void *eip);

// syscall.c
extern void syscall_init();

#endif