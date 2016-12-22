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
	asm volatile("cli");

	move_stack(0xf0000000, 0x4000);
	pids = 0;

	task = (task_t *)kmalloc(sizeof(task_t));
	task->pid = pids++;
	task->pd = current_pd;
	task->next = 0;

	todo = task;

	asm volatile("sti");


	irq_install_handler(0, switch_task);

	uint32_t divisor = 1193180 / 50;

	// Send the command byte.
	outportb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

	// Send the frequency divisor.
	outportb(0x40, l);
	outportb(0x40, h);

}

void move_stack(uint32_t top, uint32_t size) {
	uint32_t boot_ebp, boot_esp, boot_top = get_boot_stack();

	asm volatile("mov %%esp, %0" : "=r" (boot_esp));
	asm volatile("mov %%ebp, %0" : "=r" (boot_ebp));

	uint32_t ebp = top - (boot_top - boot_ebp);
	uint32_t esp = top - (boot_top - boot_esp);

	uint32_t *old = (uint32_t *)boot_esp;
	uint32_t *new = (uint32_t *)esp;

	for (uint32_t i=top; i>= (top-size); i-=0x1000)
		map_page(i, PT_RW | PT_USER);

	uint32_t pd_addr;
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" : : "r" (pd_addr));

	for (uint32_t i=0; i<top - esp; i++) {
		if (old[i] >= boot_esp && old[i] <= boot_top)
			new[i] = top - (boot_top - old[i]);
		else
			new[i] = old[i];
	}
	

	asm volatile("mov %0, %%esp;" : : "r" (esp));
	asm volatile("mov %0, %%ebp;" : : "r" (ebp));

	tss.esp0 = top;
}

void switch_task(registers_t *regs) {
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

	// Start executing as user
	become_user(USER_DS, USER_STACK, USER_CS, (void *)0x0);
}