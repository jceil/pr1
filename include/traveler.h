#ifndef TRAVELER_H
#define TRAVELER_H

#include "dijkstra.h"

#define MAX_TRAVELERS 16

#define TRAVELER_COLORS_COUNT 8

typedef struct {
    int            src;
    int            dst;
    DijkstraResult path;
    pid_t          pid;
    /* animation state */
    int            path_idx;
    int            jump;
    int            total_jumps;
    float          timer_ms;
    float          wait_ms;
    int            done;
    /* current pixel position */
    float          x, y;
    int            waiting;  /* 1 = waiting outside node (milestone 6) */
} Traveler;

#endif