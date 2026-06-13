#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "graph.h"
#include "parser.h"
#include "dijkstra.h"
#include "gui.h"
#include "traveler.h"
#include "gui_multi.h"

static int run_m1(int argc, char *argv[]);
static int run_m2(int argc, char *argv[]);
static int run_m4(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (MILESTONE == 1) return run_m1(argc, argv);
    if (MILESTONE == 2) return run_m2(argc, argv);
    if (MILESTONE == 3) return run_m2(argc, argv);
    if (MILESTONE == 4) return run_m4(argc, argv);
    fprintf(stderr, "Unknown milestone\n");
    return EXIT_FAILURE;
}

static int run_m1(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: ./dijkstra <file>\n"); return EXIT_FAILURE; }
    int src, dst;
    Graph *g = parse_input(argv[1], &src, &dst);
    if (!g) return EXIT_FAILURE;
    DijkstraResult r = dijkstra(g, src, dst);
    if (!r.found) {
        printf("No path found\n");
    } else {
        for (int i = 0; i < r.path_len; i++) {
            if (i > 0) printf(" -> ");
            printf("%d", r.path[i]);
        }
        printf("\n%d\n", r.total_weight);
    }
    dijkstra_result_free(&r);
    graph_free(g);
    return EXIT_SUCCESS;
}

static int run_m2(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: ./sim <file>\n"); return EXIT_FAILURE; }
    int src, dst;
    Graph *g = parse_input(argv[1], &src, &dst);
    if (!g) return EXIT_FAILURE;
    DijkstraResult r = dijkstra(g, src, dst);
    if (!r.found)
        printf("No path found\n");
    else {
        for (int i = 0; i < r.path_len; i++) {
            if (i > 0) printf(" -> ");
            printf("%d", r.path[i]);
        }
        printf("\n%d\n", r.total_weight);
    }
    gui_run(g, &r);
    dijkstra_result_free(&r);
    graph_free(g);
    return EXIT_SUCCESS;
}

static int run_m4(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: ./sim <file>\n"); return EXIT_FAILURE; }
    int srcs[MAX_TRAVELERS], dsts[MAX_TRAVELERS], T = 0;
    Graph *g = parse_input_multi(argv[1], srcs, dsts, &T);
    if (!g) return EXIT_FAILURE;

    Traveler travelers[MAX_TRAVELERS];
    pid_t pids[MAX_TRAVELERS];
    for (int i = 0; i < T; i++) {
        travelers[i].src  = srcs[i];
        travelers[i].dst  = dsts[i];
        travelers[i].path = dijkstra(g, srcs[i], dsts[i]);
        travelers[i].pid  = -1;
        travelers[i].done = 0;
    }

    for (int i = 0; i < T; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
        if (pid == 0) {
            printf("[%d] started\n", getpid());
            fflush(stdout);
            while (1) pause();
            exit(0);
        }
        travelers[i].pid = pid;
        pids[i]          = pid;
    }

    gui_multi_run(g, travelers, T);

    for (int i = 0; i < T; i++) kill(pids[i], SIGTERM);
    for (int i = 0; i < T; i++) {
        waitpid(pids[i], NULL, 0);
        dijkstra_result_free(&travelers[i].path);
    }
    graph_free(g);
    return EXIT_SUCCESS;
}