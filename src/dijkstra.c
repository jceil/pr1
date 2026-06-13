#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "dijkstra.h"
#include "graph.h"


typedef struct {
    int vertex;
    int dist;
} HeapNode;

typedef struct {
    HeapNode *data;
    int       size;
    int       capacity;
} MinHeap;

static MinHeap *heap_create(int capacity)
{
    MinHeap *h = malloc(sizeof(MinHeap));
    if (!h) return NULL;
    h->data     = malloc(capacity * sizeof(HeapNode));
    h->size     = 0;
    h->capacity = capacity;
    if (!h->data) { free(h); return NULL; }
    return h;
}

static void heap_free(MinHeap *h)
{
    if (!h) return;
    free(h->data);
    free(h);
}

static void heap_swap(MinHeap *h, int a, int b)
{
    HeapNode tmp  = h->data[a];
    h->data[a]    = h->data[b];
    h->data[b]    = tmp;
}

static void heap_push(MinHeap *h, int vertex, int dist)
{
    if (h->size == h->capacity) {
        h->capacity *= 2;
        h->data = realloc(h->data, h->capacity * sizeof(HeapNode));
    }
    int i = h->size++;
    h->data[i].vertex = vertex;
    h->data[i].dist   = dist;
    /* sift up */
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].dist <= h->data[i].dist) break;
        heap_swap(h, parent, i);
        i = parent;
    }
}

static HeapNode heap_pop(MinHeap *h)
{
    HeapNode top  = h->data[0];
    h->data[0]    = h->data[--h->size];
    /* sift down */
    int i = 0;
    while (1) {
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left  < h->size && h->data[left].dist  < h->data[smallest].dist) smallest = left;
        if (right < h->size && h->data[right].dist < h->data[smallest].dist) smallest = right;
        if (smallest == i) break;
        heap_swap(h, i, smallest);
        i = smallest;
    }
    return top;
}

/* ── Dijkstra ───────────────────────────────────────────────────────────── */

DijkstraResult dijkstra(const Graph *g, int src, int dst)
{
    DijkstraResult result = {NULL, 0, 0, 0};
    int N = g->num_vertices;

    /* Special case: src == dst */
    if (src == dst) {
        result.path = malloc(sizeof(int));
        if (!result.path) return result;
        result.path[0]     = src;
        result.path_len    = 1;
        result.total_weight = 0;
        result.found       = 1;
        return result;
    }

    int *dist = malloc(N * sizeof(int));
    int *prev = malloc(N * sizeof(int));
    int *visited = calloc(N, sizeof(int));
    if (!dist || !prev || !visited) {
        free(dist); free(prev); free(visited);
        return result;
    }

    for (int i = 0; i < N; i++) {
        dist[i] = INT_MAX;
        prev[i] = -1;
    }
    dist[src] = 0;

    MinHeap *heap = heap_create(N);
    if (!heap) { free(dist); free(prev); free(visited); return result; }
    heap_push(heap, src, 0);

    while (heap->size > 0) {
        HeapNode cur = heap_pop(heap);
        int u = cur.vertex;

        if (visited[u]) continue;
        visited[u] = 1;

        if (u == dst) break;

        for (Edge *e = g->adj[u]; e; e = e->next) {
            int v = e->dst;
            if (visited[v]) continue;
            if (dist[u] != INT_MAX && dist[u] + e->weight < dist[v]) {
                dist[v] = dist[u] + e->weight;
                prev[v] = u;
                heap_push(heap, v, dist[v]);
            }
        }
    }

    heap_free(heap);

    if (dist[dst] == INT_MAX) {
        /* No path found */
        free(dist); free(prev); free(visited);
        return result;
    }

    /* Reconstruct path */
    int path_len = 0;
    for (int v = dst; v != -1; v = prev[v]) path_len++;

    int *path = malloc(path_len * sizeof(int));
    if (!path) { free(dist); free(prev); free(visited); return result; }

    int idx = path_len - 1;
    for (int v = dst; v != -1; v = prev[v]) path[idx--] = v;

    result.path         = path;
    result.path_len     = path_len;
    result.total_weight = dist[dst];
    result.found        = 1;

    free(dist);
    free(prev);
    free(visited);
    return result;
}

void dijkstra_result_free(DijkstraResult *r)
{
    if (!r) return;
    free(r->path);
    r->path     = NULL;
    r->path_len = 0;
    r->found    = 0;
}