CC=gcc
CFLAGS=-g -Wall
LFLAGS=-lm `pkg-config --libs sdl2 SDL2_image`

life: Program.o Direction.o Geometry.o Organism.o Simulator.o Visualiser.o Selectors.o
	$(CC) $^ $(CFLAGS) -o $@ $(LFLAGS)

Direction.o: Direction.c Direction.h Common.h
	$(CC) $< $(CFLAGS) -c -o $@

Geometry.o: Geometry.c Geometry.h Common.h
	$(CC) $< $(CFLAGS) -c -o $@

Program.o: Program.c Simulator.h Selectors.h Common.h
	$(CC) $< $(CFLAGS) -c -o $@

Visualiser.o: Visualiser.c Simulator.h Common.h
	$(CC) $< $(CFLAGS) -c -o $@ `pkg-config --cflags sdl2 SDL2_image`

Simulator.o: Simulator.c Simulator.h Common.h
	$(CC) $< $(CFLAGS) -c -o $@

Organism.o: Organism.c Organism.h Common.h Direction.h
	$(CC) $< $(CFLAGS) -c -o $@

Selectors.o: Selectors.c Selectors.h Common.h
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o life

.PHONY: clean