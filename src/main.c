#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "graph.h"
#include "parser.h"
#include "dijkstra.h"
#include "gui.h"
#include "traveler.h"
#include "gui_multi.h"

#define MSG_DEST -1
#define STEP_US  300000
#define WAIT_US  1000000

static int run_m1(int argc, char *argv[]);
static int run_m2(int argc, char *argv[]);
static int run_m4(int argc, char *argv[]);
static int run_m5(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (MILESTONE == 1) return run_m1(argc, argv);
    if (MILESTONE == 2) return run_m2(argc, argv);
    if (MILESTONE == 3) return run_m2(argc, argv);
    if (MILESTONE == 4) return run_m4(argc, argv);
    if (MILESTONE == 5) return run_m5(argc, argv);
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

static void child_run(int src, int dst, const char *filepath,
                      int write_fd, int start_fd)
{
    char go; read(start_fd, &go, 1); close(start_fd);
    int ss[MAX_TRAVELERS], ds[MAX_TRAVELERS], t = 0;
    Graph *g = parse_input_multi(filepath, ss, ds, &t);
    if (!g) { int v=MSG_DEST; write(write_fd,&v,sizeof(v)); close(write_fd); exit(1); }
    DijkstraResult r = dijkstra(g, src, dst);
    if (!r.found) {
        int v=MSG_DEST; write(write_fd,&v,sizeof(v));
        close(write_fd); graph_free(g); exit(0);
    }
    for (int i = 0; i < r.path_len - 1; i++) {
        write(write_fd, &r.path[i], sizeof(int));
        int w = 1;
        for (Edge *e = g->adj[r.path[i]]; e; e = e->next)
            if (e->dst == r.path[i+1]) { w = e->weight; break; }
        usleep(w * STEP_US);
        if (i > 0 && i < r.path_len - 2) usleep(WAIT_US);
    }
    int v = MSG_DEST;
    write(write_fd, &v, sizeof(v));
    close(write_fd);
    dijkstra_result_free(&r);
    graph_free(g);
    exit(0);
}

static int run_m5(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: ./sim <file>\n"); return EXIT_FAILURE; }
    int srcs[MAX_TRAVELERS], dsts[MAX_TRAVELERS], T = 0;
    Graph *g = parse_input_multi(argv[1], srcs, dsts, &T);
    if (!g) return EXIT_FAILURE;

    Traveler travelers[MAX_TRAVELERS];
    int pfds[MAX_TRAVELERS][2];
    int sfds[MAX_TRAVELERS][2];
    int do_restart = 1;

    while (do_restart) {
        /* create ALL pipes before any fork */
        for (int i = 0; i < T; i++) {
            pipe(pfds[i]);
            pipe(sfds[i]);
            fcntl(pfds[i][0], F_SETFL, O_NONBLOCK);
            travelers[i].src  = srcs[i];
            travelers[i].dst  = dsts[i];
            travelers[i].done = 0;
            travelers[i].pid  = -1;
            travelers[i].path = dijkstra(g, srcs[i], dsts[i]);
        }
        /* now fork all children */
        for (int i = 0; i < T; i++) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
            if (pid == 0) {
                for (int j = 0; j < T; j++) {
                    close(pfds[j][0]);
                    if (j != i) close(pfds[j][1]);
                    close(sfds[j][1]);
                    if (j != i) close(sfds[j][0]);
                }
                child_run(srcs[i], dsts[i], argv[1], pfds[i][1], sfds[i][0]);
            }
            travelers[i].pid = pid;
            close(pfds[i][1]);
            close(sfds[i][0]);
        }
        do_restart = gui_multi_run_m5(g, travelers, T, pfds, sfds, srcs, dsts);
        for (int i = 0; i < T; i++) {
            if (travelers[i].pid > 0) {
                kill(travelers[i].pid, SIGTERM);
                waitpid(travelers[i].pid, NULL, 0);
            }
            close(pfds[i][0]);
            close(sfds[i][1]);
        }
    }
    graph_free(g);
    return EXIT_SUCCESS;
}