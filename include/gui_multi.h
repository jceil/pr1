#ifndef GUI_MULTI_H
#define GUI_MULTI_H

#include "graph.h"
#include "traveler.h"

void gui_multi_run(const Graph *g, Traveler *travelers, int num_travelers);

int  gui_multi_run_m5(const Graph *g, Traveler *travelers, int T,
                      int pfds[][2], int sfds[][2], int *srcs, int *dsts);
#endif