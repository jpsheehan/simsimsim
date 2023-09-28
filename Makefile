CC=gcc
CFLAGS=-g -Wall `pkg-config --cflags sdl2`
LFLAGS=-lm `pkg-config --libs sdl2`

life: sim.c
	$(CC) $^ $(CFLAGS) -o $@ $(LFLAGS)
