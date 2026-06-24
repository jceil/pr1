#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <semaphore.h>
#include <sys/types.h>

#define SCHED_FCFS 0
#define SCHED_SJF  1

#define MAX_QUEUE_PER_NODE 16
#define MAX_NODES_SHARED   16

/*
 * One waiting-room entry: a traveler that wants to enter a node.
 * burst = estimated time the traveler will need on the NEXT edge
 *         after leaving this node (used by SJF to pick the "shortest job").
 */
typedef struct {
    int      used;        /* 1 = slot occupied                      */
    pid_t    pid;
    long     arrival_seq;  /* monotonically increasing arrival order  */
    int      burst;        /* shortest-job metric (next edge weight) */
    sem_t    turn_sem;      /* child blocks here; parent posts to admit */
} WaitEntry;

typedef struct {
    int       node_id;
    int       occupied;              /* 1 = someone currently inside */
    WaitEntry queue[MAX_QUEUE_PER_NODE];
} NodeQueue;

typedef struct {
    int       algorithm;             /* SCHED_FCFS or SCHED_SJF        */
    long      seq_counter;           /* global arrival counter         */
    NodeQueue nodes[MAX_NODES_SHARED];
} SchedulerState;

#endif