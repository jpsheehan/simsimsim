CC=gcc
CFLAGS=-g -Wall `pkg-config --cflags sdl2`
LFLAGS=-lm `pkg-config --libs sdl2`

life: sim.o visualiser.o
	$(CC) $^ $(CFLAGS) -o $@ $(LFLAGS)

visualiser.o: visualiser.c *.h
	$(CC) $< $(CFLAGS) -c -o $@

sim.o: sim.c *.h
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f *.o life

.PHONY: clean