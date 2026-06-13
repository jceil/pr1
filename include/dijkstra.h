#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"

typedef struct {
    int *path;         
    int  path_len;    
    int  total_weight;
    int  found;        
} DijkstraResult;

DijkstraResult dijkstra(const Graph *g, int src, int dst);
void           dijkstra_result_free(DijkstraResult *r);

#endif