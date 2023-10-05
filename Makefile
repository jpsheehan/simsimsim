CC=gcc
CFLAGS=-g -Wall `pkg-config --cflags sdl2 SDL2_image`
LFLAGS=-lm `pkg-config --libs sdl2 SDL2_image`

life: main.c Direction.o Geometry.o Organism.o Simulator.o Visualiser.o 
	$(CC) $^ $(CFLAGS) -o $@ $(LFLAGS)

Visualiser.o: Visualiser.c *.h
	$(CC) $< $(CFLAGS) -c -o $@

Simulator.o: Simulator.c *.h
	$(CC) $< $(CFLAGS) -c -o $@

Geometry.o: Geometry.c Geometry.h Direction.h
	$(CC) $< $(CFLAGS) -c -o $@

Direction.o: Direction.c Direction.h
	$(CC) $< $(CFLAGS) -c -o $@

Organism.o: Organism.c Organism.h
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o life

.PHONY: clean