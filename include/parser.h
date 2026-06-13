#ifndef PARSER_H
#define PARSER_H

#include "graph.h"

/* milestone 1-3: single src/dst query */
Graph *parse_input(const char *filepath, int *src_out, int *dst_out);

/* milestone 4-5: multiple travelers */
Graph *parse_input_multi(const char *filepath,
                         int *travelers_src,
                         int *travelers_dst,
                         int *num_travelers_out);
#endif