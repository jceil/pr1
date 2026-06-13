#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "graph.h"

Graph *parse_input(const char *filepath, int *src_out, int *dst_out)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filepath);
        return NULL;
    }

    int N, M;
    if (fscanf(f, "%d %d", &N, &M) != 2 || N <= 0 || M < 0) {
        fprintf(stderr, "Error: invalid graph header\n");
        fclose(f);
        return NULL;
    }

    Graph *g = graph_create(N, M);
    if (!g) {
        fprintf(stderr, "Error: memory allocation failed\n");
        fclose(f);
        return NULL;
    }

    for (int i = 0; i < M; i++) {
        int src, dst, weight;
        if (fscanf(f, "%d %d %d", &src, &dst, &weight) != 3) {
            fprintf(stderr, "Error: malformed edge on line %d\n", i + 2);
            graph_free(g);
            fclose(f);
            return NULL;
        }
        if (weight < 0) {
            fprintf(stderr, "Error: negative weight (%d) is not allowed\n", weight);
            graph_free(g);
            fclose(f);
            return NULL;
        }
        if (src < 0 || src >= N || dst < 0 || dst >= N) {
            fprintf(stderr, "Error: vertex index out of range (src=%d dst=%d)\n", src, dst);
            graph_free(g);
            fclose(f);
            return NULL;
        }
        graph_add_edge(g, src, dst, weight);
    }

    if (fscanf(f, "%d %d", src_out, dst_out) != 2) {
        fprintf(stderr, "Error: missing query (src dst) at end of file\n");
        graph_free(g);
        fclose(f);
        return NULL;
    }
    if (*src_out < 0 || *src_out >= N || *dst_out < 0 || *dst_out >= N) {
        fprintf(stderr, "Error: query vertices out of range\n");
        graph_free(g);
        fclose(f);
        return NULL;
    }

    fclose(f);
    return g;
}

static int read_line_skip_comments(FILE *f, char *buf, int size)
{
    while (fgets(buf, size, f)) {
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (*p != '#' && *p != '\n' && *p != '\r' && *p != '\0')
            return 1;
    }
    return 0;
}

Graph *parse_input_multi(const char *filepath, int *travelers_src,
                         int *travelers_dst, int *num_travelers_out)
{
    FILE *f = fopen(filepath, "r");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", filepath); return NULL; }

    char buf[256];
    if (!read_line_skip_comments(f, buf, sizeof(buf))) {
        fprintf(stderr, "Error: missing graph header\n"); fclose(f); return NULL;
    }
    int N, M;
    if (sscanf(buf, "%d %d", &N, &M) != 2 || N <= 0 || M < 0) {
        fprintf(stderr, "Error: invalid graph header\n"); fclose(f); return NULL;
    }
    Graph *g = graph_create(N, M);
    if (!g) { fclose(f); return NULL; }

    for (int i = 0; i < M; i++) {
        if (!read_line_skip_comments(f, buf, sizeof(buf))) {
            fprintf(stderr, "Error: not enough edges\n"); graph_free(g); fclose(f); return NULL;
        }
        int s, d, w;
        if (sscanf(buf, "%d %d %d", &s, &d, &w) != 3) {
            fprintf(stderr, "Error: malformed edge\n"); graph_free(g); fclose(f); return NULL;
        }
        if (w < 0) { fprintf(stderr, "Error: negative weight\n"); graph_free(g); fclose(f); return NULL; }
        if (s<0||s>=N||d<0||d>=N) { fprintf(stderr, "Error: vertex out of range\n"); graph_free(g); fclose(f); return NULL; }
        graph_add_edge(g, s, d, w);
    }

    if (!read_line_skip_comments(f, buf, sizeof(buf))) {
        fprintf(stderr, "Error: missing traveler count\n"); graph_free(g); fclose(f); return NULL;
    }
    int T;
    if (sscanf(buf, "%d", &T) != 1 || T <= 0) {
        fprintf(stderr, "Error: invalid traveler count\n"); graph_free(g); fclose(f); return NULL;
    }
    *num_travelers_out = T;

    for (int i = 0; i < T; i++) {
        if (!read_line_skip_comments(f, buf, sizeof(buf))) {
            fprintf(stderr, "Error: missing traveler %d\n", i); graph_free(g); fclose(f); return NULL;
        }
        if (sscanf(buf, "%d %d", &travelers_src[i], &travelers_dst[i]) != 2) {
            fprintf(stderr, "Error: malformed traveler\n"); graph_free(g); fclose(f); return NULL;
        }
    }
    fclose(f);
    return g;
}