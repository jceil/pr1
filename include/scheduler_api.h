#ifndef SCHEDULER_API_H
#define SCHEDULER_API_H

#include "scheduler.h"

/* Create (mmap) and initialize the shared scheduler state for N nodes. */
SchedulerState *scheduler_create(int num_nodes, int algorithm);

/* Destroy and unmap. */
void scheduler_destroy(SchedulerState *s, int num_nodes);

/*
 * CHILD SIDE:
 * Request entry into node `node_id`. Blocks until the parent's scheduler
 * loop admits this pid (posts turn_sem). `burst` is the estimated time
 * for the next edge (used only by SJF; ignored by FCFS).
 */
void scheduler_request_enter(SchedulerState *s, int node_id, int burst);

/*
 * CHILD SIDE:
 * Call after finishing the stay in the node, to free it for the next
 * waiting traveler.
 */
void scheduler_leave(SchedulerState *s, int node_id);

/*
 * PARENT SIDE:
 * Call once per GUI frame. Scans all node queues; for every free node
 * with at least one waiter, picks the next traveler according to the
 * configured algorithm and admits it (posts its turn_sem).
 */
void scheduler_tick(SchedulerState *s, int num_nodes);

#endif