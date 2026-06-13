CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude
ROOT    = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SRCDIR  = $(ROOT)src

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    LDFLAGS = -lraylib -lm -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    LDFLAGS = -lraylib -lm -lX11
endif

ALL_SRC = $(SRCDIR)/graph.c \
          $(SRCDIR)/dijkstra.c \
          $(SRCDIR)/parser.c \
          $(SRCDIR)/main.c \
          $(SRCDIR)/gui.c \
          $(SRCDIR)/gui_multi.c

.PHONY: all milestone1 milestone2 milestone3 milestone4 clean

all: milestone4

milestone1:
	$(CC) $(CFLAGS) -DMILESTONE=1 -o $(ROOT)dijkstra $(ALL_SRC) $(LDFLAGS)

milestone2:
	$(CC) $(CFLAGS) -DMILESTONE=2 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS)

milestone3:
	$(CC) $(CFLAGS) -DMILESTONE=3 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS)

milestone4:
	$(CC) $(CFLAGS) -DMILESTONE=4 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS)

clean:
	rm -f $(ROOT)dijkstra $(ROOT)sim