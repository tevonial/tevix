#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <core/gdt.h>
#include <memory/memory.h>
#include <driver/fs.h>
#include <task/thread.h>
#include <task/scheduler.h>

thread_t *current_thread;

thread_t *thread_init() {
	// Init pid counter
	pids = 0;

	// Create initial thread
	current_thread = (thread_t *)kmalloc(sizeof(thread_t));
	current_thread->pid = pids++;
	current_thread->pd = current_pd;
	current_thread->ring = 0;
	current_thread->cr3 = current_thread->pd->phys;

	return current_thread;
}

thread_t *construct_thread(void *start) {
	thread_t *new_thread = (thread_t *)kmalloc(sizeof(thread_t));

	new_thread->pid = pids++;
	new_thread->pd = clone_pd(current_thread->pd);
	new_thread->ring = 0;

	new_thread->esp = KSTACK;
	new_thread->ebp = KSTACK;
	new_thread->eip = (uint32_t)start;

	new_thread->eax = 0;
	new_thread->cr3 = new_thread->pd->phys;

	return new_thread;
}

void print_stack(uint32_t *stack) {
	for (int i=0; i<10; i++) {
		printf("[esp + %d] = 0x%x\n", i*4, *(stack + i));
	}
} 

void preempt() {
	if (current_thread == 0)
		return;

	thread_t *old = current_thread;

	current_thread = scheduler_next();

	// Set current page directory
	current_pd = current_thread->pd;

	// Set kernel stack pointer in tss
	if (current_thread->ring)
		tss.esp0 = current_thread->esp0;

	switch_context(old, current_thread);
}


uint32_t fork() {
	// Forked thread execution starts at eip
	uint32_t eip = get_eip();

	// Forked thread has started
	if (eip == 0)
		return current_thread->pid;

	asm volatile("cli");

	// Stack pointers for new thread
	uint32_t esp, ebp;
	asm volatile("mov %%esp, %0" : "=r" (esp));
	asm volatile("mov %%ebp, %0" : "=r" (ebp));

	thread_t *fork_thread = (thread_t *)kmalloc(sizeof(thread_t));

	fork_thread->pid = pids++;
	fork_thread->pd = clone_pd(current_thread->pd);
	fork_thread->ring = current_thread->ring;
	fork_thread->esp0 = current_thread->esp0;

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

	asm volatile("mov %%esp, %0" : "=r" (current_thread->esp0));
	tss.esp0 = current_thread->esp0;

	// Start executing as user
	become_user(USER_DS, USER_STACK, USER_CS, (void *)0x0);
}