#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "graph.h"

/* skip comment lines starting with # */
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

Graph *parse_input(const char *filepath, int *src_out, int *dst_out)
{
    FILE *f = fopen(filepath, "r");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", filepath); return NULL; }

    char buf[256];
    /* header */
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

    /* query (milestone 1-3) */
    if (read_line_skip_comments(f, buf, sizeof(buf))) {
        sscanf(buf, "%d %d", src_out, dst_out);
    }
    fclose(f);
    return g;
}

Graph *parse_input_multi(const char *filepath, int *travelers_src,
                         int *travelers_dst, int *num_travelers_out)
{
    FILE *f = fopen(filepath, "r");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", filepath); return NULL; }

    char buf[256];
    /* header */
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