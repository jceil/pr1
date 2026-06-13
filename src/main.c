#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "parser.h"
#include "dijkstra.h"

#ifndef NO_GUI
#include "gui.h"
#endif

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

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

#ifndef NO_GUI
    gui_run(g, &r);
#endif

    dijkstra_result_free(&r);
    graph_free(g);
    return EXIT_SUCCESS;
}