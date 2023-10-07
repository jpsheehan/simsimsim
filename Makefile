SRC=./src
INC=./include
OBJ=./obj
CC=gcc
CFLAGS=-g -Wall -I./$(INC)
LFLAGS=-lm `pkg-config --libs sdl2 SDL_image`

life: $(OBJ)/Program.o $(OBJ)/Direction.o $(OBJ)/Geometry.o $(OBJ)/Organism.o $(OBJ)/Simulator.o $(OBJ)/Visualiser.o $(OBJ)/Selectors.o
	$(CC) $^ $(CFLAGS) -o $@ $(LFLAGS)

$(OBJ)/Direction.o: $(SRC)/Direction.c $(INC)/Direction.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@ 

$(OBJ)/Geometry.o: $(SRC)/Geometry.c $(INC)/Geometry.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Program.o: $(SRC)/Program.c $(INC)/Simulator.h $(INC)/Selectors.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Visualiser.o: $(SRC)/Visualiser.c $(INC)/Simulator.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@ `pkg-config --cflags sdl2 SDL_image`

$(OBJ)/Simulator.o: $(SRC)/Simulator.c $(INC)/Simulator.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Organism.o: $(SRC)/Organism.c $(INC)/Organism.h $(INC)/Common.h $(INC)/Direction.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Selectors.o: $(SRC)/Selectors.c $(INC)/Selectors.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f $(OBJ)/*.o life

.PHONY: clean