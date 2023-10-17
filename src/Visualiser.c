#include "SDL_render.h"
#include "SimFeatures.h"
#include "Common.h"

#if FEATURE_VISUALISER

#include "LineGraph.h"
#include "Organism.h"
#include "Simulator.h"
#include "Visualiser.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL_ttf.h>
#if FEATURE_SAVE_IMAGES
#include <SDL_image.h>
#endif
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN_W 640
#define WIN_H 480
#define SIM_SCALE 3

sem_t visualiserReadyLock;

typedef enum {
    Stepping30FPS,
    Stepping60FPS,
    Stepping90FPS,
    SteppingWithoutDelay,
    Skipping,
} PlaySpeed;

static SDL_Window *window;
static SDL_Renderer *renderer;
static TTF_Font* font;
static TTF_Font* titleFont;
static TTF_Font* smallFont;
static uint32_t generation;
static uint32_t step;
static int paddingLeft, paddingTop, simW, simH;
static SDL_Texture* fileTexture = NULL;
static SDL_Surface* fileSurface = NULL;
static float survivalRate;
static float previousSurvivalRate = -1.0f;
static volatile bool interrupted = false;
static Simulation *sim;

static Organism* drawableOrgsRead;
static volatile Organism* drawableOrgsWrite;
static volatile Neuron* neuronBackBuffer;
static volatile NeuralConnection * connectionBackBuffer;
static volatile Gene* geneBackBuffer;
static Neuron* neuronFrontBuffer;
static NeuralConnection * connectionFrontBuffer;
static Gene* geneFrontBuffer;
static volatile bool drawableOrgsStepChanged;
static volatile bool drawableOrgsGenerationChanged;
static volatile bool drawableOrgsWriteablePopulated;
static volatile bool drawableOrgsReadablePopulated;
static volatile bool disconnected = false;
static sem_t drawableOrgsLock;
static bool paused = true;

static LineGraph survivalRatesEachStep;
static LineGraph survivalRatesEachGeneration;

static OrganismId selectedOrganism;
static volatile PlaySpeed playSpeed;

void visDrawShell(void);

void visInit(uint32_t w, uint32_t h)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Could not init SDL\n");
        exit(1);
    }

#if FEATURE_SAVE_IMAGES
    if (!IMG_Init(IMG_INIT_JPG)) {
        fprintf(stderr, "Could not init img\n");
        exit(1);
    }
#endif

    if (TTF_Init() == -1) {
        fprintf(stderr, "Could not init ttf\n");
        exit(1);
    }

    window =
        SDL_CreateWindow("Visualiser", SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Could not create renderer\n");
        exit(1);
    }

    font = TTF_OpenFont("resources/SourceSansPro-Regular.ttf", 16);
    if (font == NULL) {
        fprintf(stderr, "Could not open font\n");
        exit(1);
    }

    titleFont = TTF_OpenFont("resources/SourceSansPro-Regular.ttf", 24);
    if (titleFont == NULL) {
        fprintf(stderr, "Could not open font\n");
        exit(1);
    }

    smallFont = TTF_OpenFont("resources/SourceSansPro-Regular.ttf", 10);
    if (smallFont == NULL) {
        fprintf(stderr, "Could not open font\n");
        exit(1);
    }

    simW = w;
    simH = h;
    paddingTop = (WIN_H - (SIM_SCALE * h)) / 2;
    paddingLeft = paddingTop;
    survivalRate = 100.0f;
    selectedOrganism = -1;

    fileTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);
    int width, height;
    SDL_QueryTexture(fileTexture, NULL, NULL, &width, &height);
    fileSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

    Size graphSize = { .w = WIN_W - paddingLeft * 2 - simW * SIM_SCALE, .h = 45 };
    Pos graphPos = (Pos) {
        .x = paddingLeft * 1.5 + simW * SIM_SCALE,
        .y = paddingTop + 20 * 6
    };
    survivalRatesEachGeneration = createLineGraph(sim->maxGenerations, (SDL_Color){.r = 0, .g = 0, .b = 255, .a = 255 }, graphPos, graphSize);
    graphPos.y += 65;
    survivalRatesEachStep = createLineGraph(sim->stepsPerGeneration, (SDL_Color){.r = 255, .g = 0, .b = 0, .a = 255 }, graphPos, graphSize);

    neuronBackBuffer = calloc(MAX_NEURONS * sim->population, sizeof(Neuron));
    connectionBackBuffer = calloc(MAX_CONNECTIONS * sim->population, sizeof(NeuralConnection));
    geneBackBuffer = calloc(sim->numberOfGenes * sim->population, sizeof(Gene));

    neuronFrontBuffer = calloc(MAX_NEURONS * sim->population, sizeof(Neuron));
    connectionFrontBuffer = calloc(MAX_CONNECTIONS * sim->population, sizeof(NeuralConnection));
    geneFrontBuffer = calloc(sim->numberOfGenes * sim->population, sizeof(Gene));
    
    visDrawShell();
    SDL_RenderPresent(renderer);
}

void drawTextF(TTF_Font* font, Pos pos, SDL_Color color, const char* format, va_list args)
{
    char buffer[128] = { 0 };
    SDL_Rect sourceRect, destRect;
    SDL_Surface* textSurface;
    SDL_Texture* textTexture;

    vsnprintf(buffer, 128, format, args);
    textSurface = TTF_RenderUTF8_Shaded(font, buffer, color, (SDL_Color){ .r = 255, .g = 255, .b = 255, .a = 255 });
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    sourceRect = (SDL_Rect) {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };
    destRect = (SDL_Rect) {
        .x = pos.x, pos.y, .w = textSurface->w, .h = textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, &sourceRect, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void drawTextAt(TTF_Font* font, Pos pos, SDL_Color color, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    drawTextF(font, pos, color, format, args);
    va_end(args);
}

void drawShellText(int row, SDL_Color color, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    drawTextF(font, (Pos) {
        .x = paddingLeft * 1.5 + simW * SIM_SCALE, paddingTop + 20 * row
    }, color, format, args);
    va_end(args);
}

void visSendDisconnected(void)
{
    disconnected = true;
}

void visDrawShell(void)
{
    // clear the window
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // draw outline for the play field
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &(SDL_Rect) {
        .x = paddingLeft - 1,
        .y = paddingTop - 1,
        .w = simW * SIM_SCALE + 2,
        .h = simH * SIM_SCALE + 2
    });

    SDL_Color black = { .r = 0, .g = 0, .b = 0, .a = 255 };
    SDL_Color gray = { .r = 128, .g = 128, .b = 128, .a = 255 };
    SDL_Color lightGray = { .r = 192, .g = 192, .b = 192, .a = 255 };
    // SDL_Color red = { .r = 255, .g = 0, .b = 0, .a = 255 };
    // SDL_Color blue = { .r = 0, .g = 0, .b = 255, .a = 255 };

    drawTextAt(titleFont, (Pos) {
        .x = paddingLeft, .y = 10
    }, black, "Simulation");

    if (disconnected) {
        drawShellText(0, black, "State: Disconnected");
    } else if (paused) {
        drawShellText(0, black, "State: Paused");
    } else if (playSpeed == Stepping30FPS) {
        drawShellText(0, black, "State: Speed I");
    } else if (playSpeed == Stepping60FPS) {
        drawShellText(0, black, "State: Speed II");
    } else if (playSpeed == Stepping90FPS) {
        drawShellText(0, black, "State: Speed III");
    } else if (playSpeed == SteppingWithoutDelay) {
        drawShellText(0, black, "State: Speed \u221E");
    } else if (playSpeed == Skipping) {
        drawShellText(0, black, "State: Speed \u221E + 1");
    }

    drawShellText(1, black, "Generation: %'d", generation + 1);

    if (playSpeed != Skipping) {
        drawShellText(2, black, "Step: %03d", step + 1);
        drawShellText(3, black, "Survival Rate: %.2f%%", survivalRate);
    }

    if (generation > 0) {
        drawShellText(4, black, "Prev. Rate: %.2f%%", previousSurvivalRate);
    }

    drawShellText(12, gray, "Selection: %s", sim->selector.name);
    drawShellText(13, gray, "Seed: %'d", sim->seed);
    drawShellText(14, gray, "Int. Neurons: %d", sim->maxInternalNeurons);
    drawShellText(15, gray, "No. of Genes: %d", sim->numberOfGenes);
    drawShellText(16, gray, "Mut. Rate: %.2f%%", sim->mutationRate * 100.0f);
    drawShellText(17, gray, "Gen. Pop.: %'d", sim->population);
    drawShellText(18, gray, "Gen. Count: %'d", sim->maxGenerations);

    drawTextAt(font, (Pos) {
        .x = paddingLeft, .y = WIN_H - 35
    }, black, "[ESC] = Quit");

    drawTextAt(font, (Pos) {
        .x = paddingLeft + simW * 1 * SIM_SCALE / 3, .y = WIN_H - 35
    }, disconnected ? lightGray : black, "[SPC] = %s", paused ? "Play" : "Pause");
    drawTextAt(font, (Pos) {
        .x = paddingLeft + simW * 2 * SIM_SCALE / 3, .y = WIN_H - 35
    }, disconnected || paused || (playSpeed == Stepping30FPS) ? lightGray : black, "[,] = Slower");
    drawTextAt(font, (Pos) {
        .x = paddingLeft + simW * 3 * SIM_SCALE / 3, .y = WIN_H - 35
    }, disconnected || paused || (playSpeed == Skipping) ? lightGray : black, "[.] = Faster");

    if (playSpeed != Skipping) {
        renderLineGraph(&survivalRatesEachStep, renderer);
        drawTextAt(smallFont, (Pos){.x = survivalRatesEachStep.pos.x + 2, .y = survivalRatesEachStep.pos.y - 16}, black, "Survival Rate (per Step)");
    }

    renderLineGraph(&survivalRatesEachGeneration, renderer);
    drawTextAt(smallFont, (Pos){.x = survivalRatesEachGeneration.pos.x + 2, .y = survivalRatesEachGeneration.pos.y - 16}, black, "Survival Rate (per Generation)");

    // obstacles
    for (int i = 0; i < sim->obstaclesCount; i++) {
        Rect *r = &sim->obstacles[i];
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer,
        &(SDL_Rect) {
            .x = paddingLeft + r->x * SIM_SCALE,
            .y = paddingTop + r->y * SIM_SCALE,
            .w = r->w * SIM_SCALE,
            .h = r->h * SIM_SCALE
        });
    }
}

void handleEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_WINDOWEVENT:
            switch (e.window.type) {
            case SDL_WINDOWEVENT_CLOSE:
                interrupted = true;
                break;
            }
            break;
        case SDL_KEYDOWN:
            switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                interrupted = true;
                break;
            case SDLK_SPACE:
                if (disconnected) break;
                if (paused) {
                    simSendContinue();
                } else {
                    simSendPause();
                }
                paused = !paused;
                // printf("Visualiser: Pause %s\n", paused ? "Enabled" : "Disabled");
                break;
            case SDLK_COMMA:
                if (disconnected || paused || (playSpeed == Stepping30FPS)) break;
                playSpeed--;
                break;
            case SDLK_PERIOD:
                if (disconnected || paused || (playSpeed == Skipping)) break;
                playSpeed++;
                break;
            }
            break;
        }
    }
}

SDL_Color getOrganismBaseColor(Organism* org)
{
    // alive and mutated = bright green
    if (org->alive && org->mutated) {
        return (SDL_Color) {
            .r = 0, .g = 255, .b = 0, .a = 255
        };
    }

    // alive and not-mutated = white
    if (org->alive && !org->mutated) {
        return (SDL_Color) {
            .r = 255, .g = 255, .b = 255, .a = 255
        };
    }

    // not-alive and mutated = gray
    if (!org->alive && org->mutated) {
        return (SDL_Color) {
            .r = 128, .g = 128, .b = 128, .a = 255
        };
    }

    // not alive, not mutated gray
    return (SDL_Color) {
        .r = 128, .g = 128, .b = 128, .a = 255
    };
}

void setRenderDrawColor(SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void copyBackbufferToFrontBuffer(void)
{
    sem_wait(&drawableOrgsLock);

    for (int i = 0; i < sim->population; i++) {
        destroyOrganism(&drawableOrgsRead[i]);
        drawableOrgsRead[i] = copyOrganism((Organism*)&drawableOrgsWrite[i], &neuronFrontBuffer[i * MAX_NEURONS], &connectionFrontBuffer[i * MAX_CONNECTIONS], &geneFrontBuffer[i * sim->numberOfGenes]);
    }
    drawableOrgsGenerationChanged = false;
    drawableOrgsStepChanged = false;
    drawableOrgsReadablePopulated = true;

    sem_post(&drawableOrgsLock);
}

void visDrawStep(void)
{
    handleEvents();

    if (interrupted) return;

    if ((playSpeed == Skipping && drawableOrgsStepChanged) ||
        (playSpeed != Skipping && (drawableOrgsGenerationChanged || drawableOrgsStepChanged))) {
        copyBackbufferToFrontBuffer();
    }

    SDL_SetRenderTarget(renderer, fileTexture);

    visDrawShell();

    if (drawableOrgsReadablePopulated) {
        // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (int i = 0; i < sim->population; i++) {
            Organism *org = &drawableOrgsRead[i];
            SDL_Color color = getOrganismBaseColor(org);

            setRenderDrawColor(color);
            SDL_RenderFillRect(renderer, &(SDL_Rect){
                .x = paddingLeft + SIM_SCALE * org->pos.x,
                .y = paddingTop + SIM_SCALE * org->pos.y,
                .w = SIM_SCALE,
                .h = SIM_SCALE
            });
            // SDL_RenderDrawPoint(renderer, paddingLeft + SIM_SCALE * org->pos.x + 1, paddingTop + SIM_SCALE * org->pos.y + 1);

            // SDL_Point fullPoints[4] = {
            //     (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 1, .y = paddingTop + SIM_SCALE * org->pos.y},
            //     (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 1, .y = paddingTop + SIM_SCALE * org->pos.y + 2},
            //     (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y + 1},
            //     (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y + 1},
            // };
            // color.a = 128;
            // setRenderDrawColor(color);
            // SDL_RenderDrawPoints(renderer, fullPoints, 4);

            // fullPoints[0] = (SDL_Point) {
            //     .x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y
            // };
            // fullPoints[1] = (SDL_Point) {
            //     .x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y + 2
            // };
            // fullPoints[2] = (SDL_Point) {
            //     .x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y + 2
            // };
            // fullPoints[3] = (SDL_Point) {
            //     .x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y
            // };
            // color.a = 32;
            // setRenderDrawColor(color);
            // SDL_RenderDrawPoints(renderer, fullPoints, 4);
        }
        // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    SDL_RenderPresent(renderer);

#if FEATURE_SAVE_IMAGES
    if (forceDraw) {
        char filename[128] = { '\0' };
        snprintf(filename, 128, "images/%d_%08d_%03d.jpg", seed, generation, step);

        SDL_Texture* target = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, fileTexture);
        SDL_RenderReadPixels(renderer, NULL, fileSurface->format->format, fileSurface->pixels, fileSurface->pitch);
        IMG_SavePNG(fileSurface, filename);
        SDL_SetRenderTarget(renderer, target);
    }
#endif

    SDL_SetRenderTarget(renderer, NULL);
    SDL_Rect rect = { .x = 0, .y = 0, .w = WIN_W, .h = WIN_H};
    SDL_RenderCopy(renderer, fileTexture, &rect, &rect);
    SDL_RenderPresent(renderer);
}

void visDestroy(void)
{
    destroyLineGraph(&survivalRatesEachGeneration);
    destroyLineGraph(&survivalRatesEachStep);

    free((void*)neuronBackBuffer);
    neuronBackBuffer = NULL;

    free((void*)connectionBackBuffer);
    connectionBackBuffer = NULL;

    free((void*)geneBackBuffer);
    geneBackBuffer = NULL;

    free((void*)geneFrontBuffer);
    geneFrontBuffer = NULL;

    free(neuronFrontBuffer);
    neuronFrontBuffer = NULL;

    free(connectionFrontBuffer);
    connectionFrontBuffer = NULL;

    SDL_FreeSurface(fileSurface);
    fileSurface = NULL;

    SDL_DestroyTexture(fileTexture);
    fileTexture = NULL;

    TTF_CloseFont(font);
    font = NULL;

    TTF_CloseFont(titleFont);
    titleFont = NULL;

    TTF_CloseFont(smallFont);
    smallFont = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(window);
    window = NULL;

    TTF_Quit();
#if FEATURE_SAVE_IMAGES
    IMG_Quit();
#endif
    SDL_Quit();
}

float calculateSurvivalRate(Organism* orgs)
{
    int survivors = 0;
    for (int i = 0; i < sim->population; i++) {
        if (orgs[i].alive) {
            survivors++;
        }
    }
    return 100.0f * (float)survivors / (float)sim->population;
}

void copyOrganismsToBackbuffer(Organism* orgs)
{
    sem_wait(&drawableOrgsLock);

    if (drawableOrgsWriteablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
    }

    for (int i = 0; i < sim->population; i++) {
        drawableOrgsWrite[i] = copyOrganism(&orgs[i], (Neuron*)&neuronBackBuffer[i * MAX_NEURONS], (NeuralConnection*)&connectionBackBuffer[i * MAX_CONNECTIONS], (Gene*)&geneBackBuffer[i * sim->numberOfGenes]);
    }

    drawableOrgsWriteablePopulated = true;

    sem_post(&drawableOrgsLock);
}

void visSendGeneration(Organism *orgs, int g)
{
    TRACE_BEGIN;

    if (interrupted) {
        TRACE_END;
        return;
    }

    copyOrganismsToBackbuffer(orgs);

    if (playSpeed != Skipping) {
        drawableOrgsStepChanged = true;
    }
    drawableOrgsGenerationChanged = true;

    generation = g;
    step = 0;
    selectedOrganism = -1;

    if (generation > 0) {
        previousSurvivalRate = survivalRate;
    }

    survivalRate = 100.0f;

    if (playSpeed != SteppingWithoutDelay && playSpeed != Skipping) {
        simSendFramePause();
    }

    TRACE_END;
}

void visSendQuit(void)
{
    TRACE_BEGIN;
    interrupted = true;
    TRACE_END;
}

void visSendStep(Organism* orgs, int s)
{
    TRACE_BEGIN;

    if (interrupted) {
        TRACE_END;
        return;
    }

    if (playSpeed == Skipping && s != sim->stepsPerGeneration - 1) {
        TRACE_END;
        return;
    }

    sem_wait(&drawableOrgsLock);

    if (drawableOrgsWriteablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            // this time only change the data that can change during execution
            copyOrganismMutableState((Organism*)&drawableOrgsWrite[i], &orgs[i]);
        }
        drawableOrgsStepChanged = true;

        step = s;

        survivalRate = calculateSurvivalRate((Organism*)drawableOrgsWrite);

        setPointOnGraph(&survivalRatesEachStep, step, survivalRate / 100.0f);
        setPointOnGraph(&survivalRatesEachGeneration, generation, survivalRate / 100.0f);
    }

    sem_post(&drawableOrgsLock);

    if (playSpeed == Skipping) {
        copyBackbufferToFrontBuffer();
    }

    if (playSpeed != SteppingWithoutDelay && playSpeed != Skipping) {
        simSendFramePause();
    }

    TRACE_END;
}

void runUserInterface(Simulation* s)
{
    sim = s;

    drawableOrgsWrite = calloc(sim->population, sizeof(Organism));
    drawableOrgsRead = calloc(sim->population, sizeof(Organism));
    sem_init(&drawableOrgsLock, 0, 1);

    drawableOrgsStepChanged = false;
    drawableOrgsGenerationChanged = false;
    drawableOrgsWriteablePopulated = false;
    drawableOrgsReadablePopulated = false;
    playSpeed = Stepping30FPS;

    visInit(sim->size.w, sim->size.h);

    simSendReady();
    sem_wait(&visualiserReadyLock);

    if (paused) {
        simSendPause();
    }

    while (!interrupted) {
        visDrawStep();

        if (playSpeed == Stepping30FPS) {
            SDL_Delay(1000 / 30);
        } else if (playSpeed == Stepping60FPS) {
            SDL_Delay(1000 / 60);
        } else if (playSpeed == Stepping60FPS) {
            SDL_Delay(1000 / 120);
        }

        if (!paused && playSpeed != SteppingWithoutDelay && playSpeed != Skipping) {
            simSendFrameContinue();
        }
    }

    if (paused) {
        simSendContinue();
    }
    simSendQuit();

    if (drawableOrgsReadablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism(&drawableOrgsRead[i]);
        }
        drawableOrgsReadablePopulated = false;
        free(drawableOrgsRead);
        drawableOrgsRead = NULL;
    }

    if (drawableOrgsWriteablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
        drawableOrgsWriteablePopulated = false;
        free((void*)drawableOrgsWrite);
        drawableOrgsWrite = NULL;
    }

    sem_destroy(&drawableOrgsLock);

    visDestroy();
}

void visSendReady(void)
{
    sem_post(&visualiserReadyLock);
    // printf("visSendReady() #unlocked\n");
}

#else

void visSendGeneration(Organism *orgs, int generation) {}
void visSendStep(Organism *orgs, int step) {}
void visSendQuit(void) {}
void visSendReady(void) {}
void runUserInterface(Simulation *sim) {}
void visSendDisconnected(void) {}

#endif
