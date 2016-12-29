#include <task/scheduler.h>

scheduler_queue_t *queue;

void multitask_init() {
	// Move kernel stack to uniform location
	move_stack(0xf0000000, 0x4000);

	// Initialize and schedule first thread
	queue = (scheduler_queue_t *)kmalloc(sizeof(scheduler_queue_t));
	scheduler_add(thread_init());

	// Initialize PIT for task switching
	irq_install_handler(0, preempt);

	uint32_t divisor = 1193180 / 50;
	
	outportb(0x43, 0x36);					// Command byte.

	outportb(0x40, divisor & 0xFF);			// Low byte
	outportb(0x40, (divisor>>8) & 0xFF);	// High byte
}

void scheduler_add(thread_t *thread) {
	// queue->thread[queue->total++] = thread;
	// printf("scheduler_add total: %d\n", queue->total);

	thread->next = thread_queue;
	thread_queue = thread;
}

thread_t *scheduler_next() {
	// if (++queue->current >= queue->total)
	// 	queue->current = 0;

	// return queue->thread[queue->current];

	thread_t *next;
	next = thread->next;
	if (next == 0)
		next = thread_queue;

	return next;
}