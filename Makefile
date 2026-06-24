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
          $(SRCDIR)/gui_multi.c \
          $(SRCDIR)/scheduler.c

.PHONY: all milestone1 milestone2 milestone3 milestone4 milestone5 milestone6 milestone7 clean

all: milestone3

install-raylib:
	sudo apt update
	sudo apt install -y build-essential cmake git \
		libx11-dev libxrandr-dev libxi-dev \
		libgl1-mesa-dev libxcursor-dev libxinerama-dev
	rm -rf /tmp/raylib
	git clone https://github.com/raysan5/raylib.git /tmp/raylib
	cd /tmp/raylib && mkdir -p build
	cd /tmp/raylib/build && cmake ..
	cd /tmp/raylib/build && make
	cd /tmp/raylib/build && sudo make install
	sudo ldconfig



milestone1:
	$(CC) $(CFLAGS) -DMILESTONE=1 -o $(ROOT)dijkstra $(ALL_SRC) $(LDFLAGS) -lpthread

milestone2:
	$(CC) $(CFLAGS) -DMILESTONE=2 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS) -lpthread

milestone3:
	$(CC) $(CFLAGS) -DMILESTONE=3 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS) -lpthread

milestone4:
	$(CC) $(CFLAGS) -DMILESTONE=4 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS) -lpthread

milestone5:
	$(CC) $(CFLAGS) -DMILESTONE=5 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS) -lpthread

milestone6:
	$(CC) $(CFLAGS) -DMILESTONE=6 -o $(ROOT)sim $(ALL_SRC) $(LDFLAGS) -lpthread

milestone7:
	$(CC) $(CFLAGS) -DMILESTONE=7 -o $(ROOT)sim-schd $(ALL_SRC) $(LDFLAGS) -lpthread

clean:
	rm -f $(ROOT)dijkstra $(ROOT)sim $(ROOT)sim-schda