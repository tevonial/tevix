#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <core/gdt.h>
#include <memory/memory.h>
#include <driver/fs.h>
#include <task/task.h>

task_t *task;

void task_init() {
	move_stack(0xf0000000, 0x4000);
	pids = 0;

	// First and current thread
	task = (task_t *)kmalloc(sizeof(task_t));
	task->pid = pids++;
	task->pd = current_pd;
	task->ring = 0;
	task->next = 0;

	todo = task;


	// Initialize PIT for task switching
	irq_install_handler(0, switch_task);

	uint32_t divisor = 1193180 / 50;
	
	outportb(0x43, 0x36);					// Command byte.

	outportb(0x40, divisor & 0xFF);			// Low byte
	outportb(0x40, (divisor>>8) & 0xFF);	// High byte

}

void move_stack(uint32_t top, uint32_t size) {
	extern void _init_stack_start();
	extern void _init_stack_end();

	asm volatile("cli");

	uint32_t init_ebp, init_esp;
	asm volatile("mov %%esp, %0" : "=r" (init_esp));
	asm volatile("mov %%ebp, %0" : "=r" (init_ebp));

	uint32_t ebp = top - ((uint32_t)_init_stack_end - init_ebp);
	uint32_t esp = top - ((uint32_t)_init_stack_end - init_esp);

	uint32_t *old = (uint32_t *)init_esp;
	uint32_t *new = (uint32_t *)esp;

	for (uint32_t i=top; i>= (top-size); i-=0x1000)
		map_page(i, PT_RW | PT_USER);

	uint32_t pd_addr;
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

	for (uint32_t i=0; i<top - esp; i++) {
		if (old[i] >= init_esp && old[i] <= _init_stack_end)
			new[i] = top - ((uint32_t)_init_stack_end - old[i]);
		else
			new[i] = old[i];
	}
	
	// New stack location open for business
	asm volatile("mov %0, %%esp;" : : "r" (esp));
	asm volatile("mov %0, %%ebp;" : : "r" (ebp));

	// Clear and add old stack space to the heap
	memset(_init_stack_start, 0, _init_stack_end - _init_stack_start);
	heap_list_add(_init_stack_start, _init_stack_end - _init_stack_start);

	asm volatile("sti");
}

void switch_task(registers_t *regs) {
	static int x = 0;

	if (x++ % 20 > 0)
		return;

	if (task == 0)
		return;

	uint32_t eip = get_eip();
	if (eip == 1) {
	  	return;	// New task is has now started
	}

	task->eip = eip;
	asm volatile("mov %%esp, %0" : "=r" (task->esp));
	asm volatile("mov %%ebp, %0" : "=r" (task->ebp));


	task = task->next;
	if (task == 0)
		task = todo;

	// Set current page directory
	current_pd = task->pd;

	// Set kernel stack pointer in tss
	if (task->ring)
		tss.esp0 = task->esp0;

	task->fresh = true;

	printf("PID %d esp0 %x\n", task->pid, task->esp0);

	switch_context(task->eip, current_pd->phys, task->ebp, task->esp); 
}

uint32_t fork() {
	uint32_t esp, ebp;
	asm volatile("mov %%esp, %0" : "=r" (esp));
	asm volatile("mov %%ebp, %0" : "=r" (ebp));

	uint32_t eip = get_eip();

	if (eip == 1) {
		return 0;
	}

	asm volatile("cli");

	task_t *new = (task_t *)kmalloc(sizeof(task_t));

	new->pid = pids++;
	new->pd = clone_pd(task->pd);
	new->ring = task->ring;
	new->esp0 = task->esp0;

	new->esp = esp;
	new->ebp = ebp;
	new->eip = eip;

	new->next = todo;
	todo = new;


	asm volatile("sti");

	return new->pid;

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

	asm volatile("mov %%esp, %0" : "=r" (task->esp0));
	tss.esp0 = task->esp0;

	// Start executing as user
	become_user(USER_DS, USER_STACK, USER_CS, (void *)0x0);
}