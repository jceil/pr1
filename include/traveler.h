#ifndef TRAVELER_H
#define TRAVELER_H

#include "dijkstra.h"
#include <sys/types.h>

#define MAX_TRAVELERS 16

typedef struct {
    int            src;
    int            dst;
    DijkstraResult path;
    pid_t          pid;
    int            path_idx;
    int            jump;
    int            total_jumps;
    float          timer_ms;
    float          wait_ms;
    int            done;
    float          x, y;
} Traveler;

#endif