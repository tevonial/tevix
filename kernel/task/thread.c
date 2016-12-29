#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <core/gdt.h>
#include <memory/memory.h>
#include <driver/fs.h>
#include <task/thread.h>
#include <task/scheduler.h>

thread_t *thread;

thread_t *thread_init() {
	// Init pid counter
	pids = 0;

	// Create initial thread
	thread = (thread_t *)kmalloc(sizeof(thread_t));
	thread->pid = pids++;
	thread->pd = current_pd;
	thread->ring = 0;
	thread->cr3 = thread->pd->phys;

	return thread;
}

void move_stack(uint32_t top, uint32_t size) {
	extern void _init_stack_start();
	extern void _init_stack_end();

	uint32_t init_ebp, init_esp;
	asm volatile("mov %%esp, %0" : "=r" (init_esp));
	asm volatile("mov %%ebp, %0" : "=r" (init_ebp));

	uint32_t ebp = top - ((uint32_t)_init_stack_end - init_ebp);
	uint32_t esp = top - ((uint32_t)_init_stack_end - init_esp);

	uint32_t *old = (uint32_t *)init_esp;
	uint32_t *new = (uint32_t *)esp;

	for (uint32_t i=top; i>= (top-size); i-=0x1000)
		map_page(i, PT_RW | PT_USER);

	for (uint32_t i=0; i<top - esp; i++) {
		if (old[i] >= init_esp && old[i] <= _init_stack_end)
			new[i] = top - ((uint32_t)_init_stack_end - old[i]);
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

void preempt(registers_t *regs) {
	if (thread == 0)
		return;

	thread_t *old = thread;

	thread = scheduler_next();

	// Set current page directory
	current_pd = thread->pd;

	// Set kernel stack pointer in tss
	if (thread->ring)
		tss.esp0 = thread->esp0;

	switch_context(old, thread);
}

void print_stack(uint32_t *stack) {
	for (int i=0; i<10; i++) {
		printf("[esp + %d] = 0x%x\n", i*4, *(stack + i));
	}
} 

uint32_t fork() {
	// Forked thread execution starts at eip
	uint32_t eip = get_eip();

	// Forked thread has started
	if (eip == 0)
		return thread->pid;

	asm volatile("cli");

	// Stack pointers for new thread
	uint32_t esp, ebp;
	asm volatile("mov %%esp, %0" : "=r" (esp));
	asm volatile("mov %%ebp, %0" : "=r" (ebp));

	thread_t *fork_thread = (thread_t *)kmalloc(sizeof(thread_t));

	fork_thread->pid = pids++;
	fork_thread->pd = clone_pd(thread->pd);
	fork_thread->ring = thread->ring;
	fork_thread->esp0 = thread->esp0;

	fork_thread->esp = esp + 0;
	fork_thread->ebp = ebp;
	fork_thread->eip = eip;

	fork_thread->eax = 0;
	fork_thread->cr3 = fork_thread->pd->phys;

	scheduler_add(fork_thread);

	asm volatile("sti");
	return fork_thread->pid;
}

void exec(char *name) {
	fs_node_t *node = fs_finddir(fs_root, name);

	if (!node)
		return;

	// Allocate memory for user text
	for (uint32_t i=0; i<node->length; i+=0x1000)
		map_page(i, PT_RW | PT_USER);

	// Allocate memory for user stack
	map_page(USER_STACK, PT_RW | PT_USER);

	// Load binary to 0x00000000
	fs_read(node, 0, node->length, (void *)0);

	asm volatile("mov %%esp, %0" : "=r" (thread->esp0));
	tss.esp0 = thread->esp0;

	// Start executing as user
	become_user(USER_DS, USER_STACK, USER_CS, (void *)0x0);
}