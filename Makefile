SRC=./src
INC=./include
OBJ=./obj
CC=gcc
CFLAGS=-Wall -I$(INC)
CFLAGS_DEBUG=-g
CFLAGS_RELEASE=-Ofast
LFLAGS=-lm -lpthread
SDL_LFLAGS=`pkg-config --libs   sdl2 SDL2_image SDL2_ttf`
SDL_CFLAGS=`pkg-config --cflags sdl2 SDL2_image SDL2_ttf`
SEED=123123
EXE=./life

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(EXE)

release: CFLAGS += $(CFLAGS_RELEASE)
release: clean $(EXE)

$(EXE): $(OBJ)/Program.o $(OBJ)/Direction.o $(OBJ)/Geometry.o $(OBJ)/Arena.o $(OBJ)/Organism.o $(OBJ)/Simulator.o $(OBJ)/Visualiser.o $(OBJ)/Selectors.o $(OBJ)/NeuralNet.o $(OBJ)/Genome.o
	$(CC) $^ $(CFLAGS) -o $@ $(LFLAGS) $(SDL_LFLAGS)

$(OBJ)/Direction.o: $(SRC)/Direction.c $(INC)/Direction.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@ 

$(OBJ)/Geometry.o: $(SRC)/Geometry.c $(INC)/Geometry.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Program.o: $(SRC)/Program.c $(INC)/Simulator.h $(INC)/Selectors.h $(INC)/Common.h $(INC)/Features.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Visualiser.o: $(SRC)/Visualiser.c $(INC)/Simulator.h $(INC)/Common.h $(INC)/Features.h
	$(CC) $< $(CFLAGS) -c -o $@ $(SDL_CFLAGS)

$(OBJ)/Simulator.o: $(SRC)/Simulator.c $(INC)/Simulator.h $(INC)/Common.h $(INC)/Features.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Organism.o: $(SRC)/Organism.c $(INC)/Organism.h $(INC)/Common.h $(INC)/Direction.h $(INC)/Genome.h $(INC)/NeuralNet.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Selectors.o: $(SRC)/Selectors.c $(INC)/Selectors.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/NeuralNet.o: $(SRC)/NeuralNet.c $(INC)/NeuralNet.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Genome.o: $(SRC)/Genome.c $(INC)/Genome.h $(INC)/Common.h
	$(CC) $< $(CFLAGS) -c -o $@

$(OBJ)/Arena.o: $(SRC)/Arena.c $(INC)/Arena.h
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm -f $(OBJ)/*.o $(EXE)

cachegrind: $(EXE)
	valgrind --tool=cachegrind $(EXE) $(SEED)

callgrind: $(EXE)
	valgrind --tool=callgrind $(EXE) $(SEED)

format:
	astyle --style=kr --recursive ./*.c,*.h

.PHONY: clean cachegrind callgrind format