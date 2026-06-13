#ifndef GRAPH_H
#define GRAPH_H

typedef struct Edge {
    int dst;
    int weight;
    struct Edge *next;
} Edge;

typedef struct {
    int      num_vertices;
    int      num_edges;
    Edge   **adj;       
} Graph;

Graph *graph_create(int num_vertices, int num_edges);
void   graph_add_edge(Graph *g, int src, int dst, int weight);
void   graph_free(Graph *g);

#endif