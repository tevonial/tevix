#include <memory/paging.h>
#include <task/scheduler.h>

scheduler_queue_t *queue;

void multitask_init() {
	// Move kernel stack to uniform location
	move_stack(KSTACK, KSTACK_LIM);

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

void scheduler_remove(thread_t *thread) {
	// If thread if the first in queue
	if (thread == thread_queue) {
		thread_queue = thread_queue->next;
		return;
	}

	thread_t *iterator = thread_queue;
	while (iterator) {
		if (iterator->next == thread)
			iterator->next = iterator->next->next;
		else
			iterator = iterator->next;
	}

	// Skip thread immediately if running
	if (thread == current_thread)
		preempt();
}

thread_t *scheduler_next() {
	// if (++queue->current >= queue->total)
	// 	queue->current = 0;

	// return queue->thread[queue->current];

	thread_t *next;
	next = current_thread->next;
	if (next == 0)
		next = thread_queue;

	return next;
}