CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# milestone 1 compiles to 'dijkstra'
milestone1: main.c
	$(CC) $(CFLAGS) main.c -o dijkstra $(LDFLAGS)

# milestone 2 compiles to 'sim'
milestone2: main.c
	$(CC) $(CFLAGS) main.c -o sim $(LDFLAGS)

# milestone 3 compiles to 'sim'
milestone3: main.c
	$(CC) $(CFLAGS) main.c -o sim $(LDFLAGS)

# make clean cleans all compiled files
clean:
	rm -f dijkstra sim sim-schd