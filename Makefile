CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude
LDFLAGS = -lraylib -lm -lX11

ROOT   = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SRCDIR = $(ROOT)src

SRC_BASE = $(SRCDIR)/graph.c $(SRCDIR)/dijkstra.c $(SRCDIR)/parser.c $(SRCDIR)/main.c
SRC_GUI  = $(SRCDIR)/gui.c

.PHONY: all milestone1 milestone2 milestone3 clean

all: milestone3

milestone1:
	$(CC) $(CFLAGS) -DNO_GUI -o $(ROOT)dijkstra $(SRC_BASE) -lm

milestone2 milestone3:
	$(CC) $(CFLAGS) -o $(ROOT)sim $(SRC_BASE) $(SRC_GUI) $(LDFLAGS)

clean:
	rm -f $(ROOT)dijkstra $(ROOT)sim