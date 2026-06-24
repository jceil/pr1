#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "scheduler_api.h"

SchedulerState *scheduler_create(int num_nodes, int algorithm)
{
    SchedulerState *s = mmap(NULL, sizeof(SchedulerState),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (s == MAP_FAILED) { perror("mmap scheduler"); return NULL; }

    memset(s, 0, sizeof(SchedulerState));
    s->algorithm   = algorithm;
    s->seq_counter = 0;

    for (int i = 0; i < num_nodes && i < MAX_NODES_SHARED; i++) {
        s->nodes[i].node_id  = i;
        s->nodes[i].occupied = 0;
        for (int j = 0; j < MAX_QUEUE_PER_NODE; j++)
            s->nodes[i].queue[j].used = 0;
    }
    return s;
}

void scheduler_destroy(SchedulerState *s, int num_nodes)
{
    if (!s) return;
    for (int i = 0; i < num_nodes && i < MAX_NODES_SHARED; i++)
        for (int j = 0; j < MAX_QUEUE_PER_NODE; j++)
            if (s->nodes[i].queue[j].used)
                sem_destroy(&s->nodes[i].queue[j].turn_sem);
    munmap(s, sizeof(SchedulerState));
}

/* find a free slot in the node's queue array */
static int find_free_slot(NodeQueue *nq)
{
    for (int i = 0; i < MAX_QUEUE_PER_NODE; i++)
        if (!nq->queue[i].used) return i;
    return -1;
}

void scheduler_request_enter(SchedulerState *s, int node_id, int burst)
{
    NodeQueue *nq = &s->nodes[node_id];
    int slot = find_free_slot(nq);
    if (slot < 0) return; /* queue full — should not happen with MAX_QUEUE_PER_NODE=16 */

    WaitEntry *e = &nq->queue[slot];
    sem_init(&e->turn_sem, 1, 0);   /* shared, starts locked */
    e->pid         = getpid();
    e->burst       = burst;
    e->arrival_seq = __sync_fetch_and_add(&s->seq_counter, 1);
    e->used        = 1;

    /* block here until parent's scheduler_tick admits us */
    sem_wait(&e->turn_sem);
}

void scheduler_leave(SchedulerState *s, int node_id)
{
    NodeQueue *nq = &s->nodes[node_id];
    nq->occupied = 0;
}

/* parent: pick the winning waiter index for a node, or -1 if none waiting */
static int pick_winner(NodeQueue *nq, int algorithm)
{
    int best = -1;
    for (int i = 0; i < MAX_QUEUE_PER_NODE; i++) {
        if (!nq->queue[i].used) continue;
        if (best == -1) { best = i; continue; }

        if (algorithm == SCHED_SJF) {
            if (nq->queue[i].burst < nq->queue[best].burst) best = i;
            else if (nq->queue[i].burst == nq->queue[best].burst &&
                     nq->queue[i].arrival_seq < nq->queue[best].arrival_seq)
                best = i;
        } else { /* FCFS */
            if (nq->queue[i].arrival_seq < nq->queue[best].arrival_seq) best = i;
        }
    }
    return best;
}

void scheduler_tick(SchedulerState *s, int num_nodes)
{
    for (int n = 0; n < num_nodes && n < MAX_NODES_SHARED; n++) {
        NodeQueue *nq = &s->nodes[n];
        if (nq->occupied) continue;  /* node busy, nothing to admit */

        int winner = pick_winner(nq, s->algorithm);
        if (winner < 0) continue;    /* nobody waiting */

        nq->occupied = 1;
        sem_post(&nq->queue[winner].turn_sem);
        /* mark slot for cleanup once the waiting child wakes and clears it */
        nq->queue[winner].used = 0;
    }
}