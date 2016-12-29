#ifndef __TASK_SCHEDULER_H
#define __TASK_SCHEDULER_H

#include <stdint.h>

#include <memory/heap.h>
#include <task/thread.h>


typedef struct {
	thread_t *thread[1000];
	uint32_t current;
	uint32_t total;
} scheduler_queue_t;

thread_t *thread_queue;

extern void multitask_init();
extern void scheduler_add(thread_t *thread);
extern void scheduler_remove(thread_t *thread);
extern thread_t *scheduler_next();

#endif