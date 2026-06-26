#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define MSG_WAIT -2
#define STEP_US  300000
#define WAIT_US  1000000

#include "scheduler_api.h"

static int run_m1(int argc, char *argv[]);
static int run_m2(int argc, char *argv[]);
static int run_m4(int argc, char *argv[]);
static int run_m5(int argc, char *argv[]);
static int run_m6(int argc, char *argv[]);
static int run_m7(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (MILESTONE == 1) return run_m1(argc, argv);
    if (MILESTONE == 2) return run_m2(argc, argv);
    if (MILESTONE == 3) return run_m2(argc, argv);
    if (MILESTONE == 4) return run_m4(argc, argv);
    if (MILESTONE == 5) return run_m5(argc, argv);
    if (MILESTONE == 6) return run_m6(argc, argv);
    if (MILESTONE == 7) return run_m7(argc, argv);
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
                      int write_fd, int start_fd, int ack_fd)
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

        //test
    //      printf("[PID=%d] waiting for ACK at node %d\n", getpid(), r.path[i]);
    // fflush(stdout);
    
        /* wait for ACK from parent before continuing */
        char ack; read(ack_fd, &ack, 1);

        //test
    //      printf("[PID=%d] got ACK, continuing\n", getpid());
    // fflush(stdout);

        /* wait 1 second at intermediate nodes BEFORE leaving */
        if (i > 0) usleep(WAIT_US);

        int w = 1;
        for (Edge *e = g->adj[r.path[i]]; e; e = e->next)
            if (e->dst == r.path[i+1]) { w = e->weight; break; }

        /* travel the edge step by step */
        for (int step = 0; step < w; step++) {
            usleep(STEP_US);
            int msg_step = -3;
            write(write_fd, &msg_step, sizeof(int)); // Send JUMP command
        }
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
    int afds[MAX_TRAVELERS][2];  /* parent->child: ACK */
    int do_restart = 1;

    while (do_restart) {
        /* create ALL pipes before any fork */
        for (int i = 0; i < T; i++) {
            pipe(pfds[i]);
            pipe(sfds[i]);
            pipe(afds[i]);
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
                    close(afds[j][1]);
                    if (j != i) close(afds[j][0]);
                }
                child_run(srcs[i], dsts[i], argv[1], pfds[i][1], sfds[i][0], afds[i][0]);
            }
            travelers[i].pid = pid;
            close(pfds[i][1]);
            close(sfds[i][0]);
            close(afds[i][0]);
        }
        do_restart = gui_multi_run_m5(g, travelers, T, pfds, sfds, afds, srcs, dsts);
        for (int i = 0; i < T; i++) {
            if (travelers[i].pid > 0) {
                kill(travelers[i].pid, SIGTERM);
                waitpid(travelers[i].pid, NULL, 0);
            }
            close(pfds[i][0]);
            close(sfds[i][1]);
            close(afds[i][1]);
        }
    }
    graph_free(g);
    return EXIT_SUCCESS;
}

/* ── milestone 6: same as 5 + semaphore per node ────────────────────── */
#include <sys/mman.h>
#include <semaphore.h>

#define MAX_NODES 15

static void child_run_m6(int src, int dst, const char *filepath,
                          int write_fd, int start_fd, sem_t *node_sems)
{
    /* wait for PLAY */
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
        int cur  = r.path[i];
        int next = r.path[i+1];

        /* notify parent: waiting to enter node */
        int msg_wait = MSG_WAIT;
        write(write_fd, &msg_wait, sizeof(int));

        /* acquire semaphore for current node (critical section) */
        sem_wait(&node_sems[cur]);

        /* notify parent: arrived at cur */
        write(write_fd, &cur, sizeof(int));

        /* stay in node for 1 second */
        usleep(WAIT_US);

        /* travel time on edge */
        int w = 1;
        for (Edge *e = g->adj[cur]; e; e = e->next)
            if (e->dst == next) { w = e->weight; break; }

        /* release node */
        sem_post(&node_sems[cur]);

        /* travel along edge step by step */
        for (int step = 0; step < w; step++) {
            usleep(STEP_US);
            int msg_step = -3;
            write(write_fd, &msg_step, sizeof(int)); // Send JUMP command to parent
        }
    }

    /* send DESTINATION */
    int v = MSG_DEST;
    write(write_fd, &v, sizeof(v));
    close(write_fd);
    dijkstra_result_free(&r);
    graph_free(g);
    exit(0);
}

static int run_m6(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "Usage: ./sim <file>\n"); return EXIT_FAILURE; }
    int srcs[MAX_TRAVELERS], dsts[MAX_TRAVELERS], T = 0;
    Graph *g = parse_input_multi(argv[1], srcs, dsts, &T);
    if (!g) return EXIT_FAILURE;

    /* shared memory semaphores — one per node */
    int N = g->num_vertices;
    sem_t *node_sems = mmap(NULL, N * sizeof(sem_t),
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (node_sems == MAP_FAILED) { perror("mmap"); return EXIT_FAILURE; }
    for (int i = 0; i < N; i++)
        sem_init(&node_sems[i], 1, 1);  /* 1 = shared, initial=1 (binary semaphore) */

    Traveler travelers[MAX_TRAVELERS];
    int pfds[MAX_TRAVELERS][2];
    int sfds[MAX_TRAVELERS][2];
    int afds[MAX_TRAVELERS][2];  /* ACK pipes */
    int do_restart = 1;

    while (do_restart) {
        /* create all pipes before forking */
        for (int i = 0; i < T; i++) {
            pipe(pfds[i]); pipe(sfds[i]); pipe(afds[i]);
            fcntl(pfds[i][0], F_SETFL, O_NONBLOCK);
            travelers[i].src  = srcs[i];
            travelers[i].dst  = dsts[i];
            travelers[i].done = 0;
            travelers[i].pid  = -1;
            travelers[i].path = dijkstra(g, srcs[i], dsts[i]);
        }
        for (int i = 0; i < T; i++) {
            pid_t pid = fork();
            if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
            if (pid == 0) {
                for (int j = 0; j < T; j++) {
                    close(pfds[j][0]);
                    if (j != i) close(pfds[j][1]);
                    close(sfds[j][1]);
                    if (j != i) close(sfds[j][0]);
                    close(afds[j][1]);
                    if (j != i) close(afds[j][0]);
                }
                child_run_m6(srcs[i], dsts[i], argv[1],
                             pfds[i][1], sfds[i][0], node_sems);
            }
            travelers[i].pid = pid;
            close(pfds[i][1]);
            close(sfds[i][0]);
            close(afds[i][0]);
        }

        do_restart = gui_multi_run_m5(g, travelers, T, pfds, sfds, afds, srcs, dsts);

        for (int i = 0; i < T; i++) {
            if (travelers[i].pid > 0) {
                kill(travelers[i].pid, SIGTERM);
                waitpid(travelers[i].pid, NULL, 0);
            }
            close(pfds[i][0]);
            close(sfds[i][1]);
            close(afds[i][1]);
            dijkstra_result_free(&travelers[i].path);
        }
        /* reset semaphores on restart */
        for (int i = 0; i < N; i++) {
            sem_destroy(&node_sems[i]);
            sem_init(&node_sems[i], 1, 1);
        }
    }

    for (int i = 0; i < N; i++) sem_destroy(&node_sems[i]);
    munmap(node_sems, N * sizeof(sem_t));
    graph_free(g);
    return EXIT_SUCCESS;
}
/* ── milestone 7: scheduling algorithms for node entry (FCFS / SJF) ──── */

static int get_weight(const Graph *g, int u, int v)
{
    for (Edge *e = g->adj[u]; e; e = e->next)
        if (e->dst == v) return e->weight;
    return 1;
}

static void child_run_m7(int src, int dst, const char *filepath,
                          int write_fd, int start_fd,
                          SchedulerState *sched)
{
    /* wait for PLAY */
    char go; read(start_fd, &go, 1); close(start_fd);

    int ss[MAX_TRAVELERS], ds[MAX_TRAVELERS], t = 0;
    Graph *g = parse_input_multi(filepath, ss, ds, &t);
    if (!g) { int v = MSG_DEST; write(write_fd, &v, sizeof(v)); close(write_fd); exit(1); }

    DijkstraResult r = dijkstra(g, src, dst);
    if (!r.found) {
        int v = MSG_DEST; write(write_fd, &v, sizeof(v));
        close(write_fd); graph_free(g); exit(0);
    }

    for (int i = 0; i < r.path_len - 1; i++) {
        int cur  = r.path[i];
        int next = r.path[i + 1];

        /* burst = time needed for the upcoming edge (used by SJF) */
        int burst = get_weight(g, cur, next) * (STEP_US / 1000);

        /* notify parent: about to wait for entry into cur */
        int msg_wait = MSG_WAIT;
        write(write_fd, &msg_wait, sizeof(int));

        /* request and block until scheduler admits us */
        scheduler_request_enter(sched, cur, burst);

        /* notify parent: now inside cur */
        write(write_fd, &cur, sizeof(int));

        /* stay in node for 1 second (critical section) */
        usleep(WAIT_US);

        /* leave the node — frees it for the scheduler to admit the next one */
        scheduler_leave(sched, cur);

        /* travel along the edge step by step */
        int w = get_weight(g, cur, next);
        for (int step = 0; step < w; step++) {
            usleep(STEP_US);
            int msg_step = -3;
            write(write_fd, &msg_step, sizeof(int));
        }
    }

    int v = MSG_DEST;
    write(write_fd, &v, sizeof(v));
    close(write_fd);
    dijkstra_result_free(&r);
    graph_free(g);
    exit(0);
}

static int run_m7(int argc, char *argv[])
{
    /* parse: ./sim -schd fcfs|sjf <file_name> */
    if (argc != 4 || strcmp(argv[1], "-schd") != 0) {
        fprintf(stderr, "Usage: %s -schd fcfs|sjf <file_name>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int algorithm;
    if (strcmp(argv[2], "fcfs") == 0)      algorithm = SCHED_FCFS;
    else if (strcmp(argv[2], "sjf") == 0)  algorithm = SCHED_SJF;
    else {
        fprintf(stderr, "Unknown scheduling algorithm '%s' (use fcfs or sjf)\n", argv[2]);
        return EXIT_FAILURE;
    }
    const char *filepath = argv[3];

    int srcs[MAX_TRAVELERS], dsts[MAX_TRAVELERS], T = 0;
    Graph *g = parse_input_multi(filepath, srcs, dsts, &T);
    if (!g) return EXIT_FAILURE;

    int N = g->num_vertices;
    SchedulerState *sched = scheduler_create(N, algorithm);
    if (!sched) { graph_free(g); return EXIT_FAILURE; }

    Traveler travelers[MAX_TRAVELERS];
    int pfds[MAX_TRAVELERS][2];
    int sfds[MAX_TRAVELERS][2];
    int afds[MAX_TRAVELERS][2];  /* ACK pipes */
    int do_restart = 1;

    while (do_restart) {
        /* create all pipes before forking */
        for (int i = 0; i < T; i++) {
            pipe(pfds[i]); pipe(sfds[i]); pipe(afds[i]);
            fcntl(pfds[i][0], F_SETFL, O_NONBLOCK);
            travelers[i].src  = srcs[i];
            travelers[i].dst  = dsts[i];
            travelers[i].done = 0;
            travelers[i].pid  = -1;
            travelers[i].path = dijkstra(g, srcs[i], dsts[i]);
        }
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
                child_run_m7(srcs[i], dsts[i], filepath,
                              pfds[i][1], sfds[i][0], sched);
            }
            travelers[i].pid = pid;
            close(pfds[i][1]);
            close(sfds[i][0]);
        }

        do_restart = gui_multi_run_m7(g, travelers, T, pfds, sfds,
                                       srcs, dsts, sched, algorithm);

        for (int i = 0; i < T; i++) {
            if (travelers[i].pid > 0) {
                kill(travelers[i].pid, SIGTERM);
                waitpid(travelers[i].pid, NULL, 0);
            }
            close(pfds[i][0]);
            close(sfds[i][1]);
            close(afds[i][1]);
            dijkstra_result_free(&travelers[i].path);
        }
        /* reset scheduler state on restart */
        scheduler_destroy(sched, N);
        sched = scheduler_create(N, algorithm);
    }

    scheduler_destroy(sched, N);
    graph_free(g);
    return EXIT_SUCCESS;
}