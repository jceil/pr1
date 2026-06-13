#include <stdio.h>
#include <stdlib.h>
#include "graph.h"

Graph *graph_create(int num_vertices, int num_edges)
{
    Graph *g = malloc(sizeof(Graph));
    if (!g) return NULL;

    g->num_vertices = num_vertices;
    g->num_edges    = num_edges;
    g->adj          = calloc(num_vertices, sizeof(Edge *));
    if (!g->adj) {
        free(g);
        return NULL;
    }
    return g;
}

void graph_add_edge(Graph *g, int src, int dst, int weight)
{
    Edge *e = malloc(sizeof(Edge));
    if (!e) {
        fprintf(stderr, "Error: memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    e->dst    = dst;
    e->weight = weight;
    e->next   = g->adj[src];
    g->adj[src] = e;
}

void graph_free(Graph *g)
{
    if (!g) return;
    for (int i = 0; i < g->num_vertices; i++) {
        Edge *cur = g->adj[i];
        while (cur) {
            Edge *tmp = cur->next;
            free(cur);
            cur = tmp;
        }
    }
    free(g->adj);
    free(g);
}