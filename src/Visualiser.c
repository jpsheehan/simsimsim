#include "Features.h"
#include "Common.h"

#if FEATURE_VISUALISER

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
#define FPS 30

sem_t visualiserReadyLock;

static SDL_Window *window;
static SDL_Renderer *renderer;
static TTF_Font* font;
static uint32_t generation;
static uint32_t step;
static int paddingLeft, paddingTop, simW, simH;
static Rect *OBSTACLES = NULL;
static int OBSTACLE_COUNT = 0;
static SDL_Texture* fileTexture = NULL;
static SDL_Surface* fileSurface = NULL;
static float survivalRate;
static float previousSurvivalRate = -1.0f;
static volatile bool interrupted = false;
static Simulation *sim;

static Organism* drawableOrgsRead;
static volatile Organism* drawableOrgsWrite;
static volatile bool drawableOrgsStepChanged;
static volatile bool drawableOrgsGenerationChanged;
static volatile bool drawableOrgsWriteablePopulated;
static volatile bool drawableOrgsReadablePopulated;
static sem_t drawableOrgsLock;
static bool paused = true;
// static bool fastPlay = false;
static bool withDelay = true;

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

    simW = w;
    simH = h;
    paddingTop = (WIN_H - (SIM_SCALE * h)) / 2;
    paddingLeft = paddingTop;// (WIN_W - (SIM_SCALE * w)) / 2;
    survivalRate = 100.0f;

    fileTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);
    // printf("Created fileTexture\n");
    int width, height;
    SDL_QueryTexture(fileTexture, NULL, NULL, &width, &height);
    fileSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

    visDrawShell();
    SDL_RenderPresent(renderer);
}

void drawShellText(int row, SDL_Color color, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[128] = { 0 };
    SDL_Rect sourceRect, destRect;
    SDL_Surface* textSurface;
    SDL_Texture* textTexture;

    vsnprintf(buffer, 128, format, args);
    textSurface = TTF_RenderText_Blended(font, buffer, color);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    sourceRect = (SDL_Rect) {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };
    destRect = (SDL_Rect) {
        .x = paddingLeft * 1.5 + simW * SIM_SCALE, paddingTop + 20 * row, .w = textSurface->w, .h = textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, &sourceRect, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    va_end(args);
}

void visDrawShell(void)
{
    // clear the window
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // draw outline for the play field
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &(SDL_Rect) {
        .x = paddingLeft - 1,
        .y = paddingTop - 1,
        .w = simW * SIM_SCALE + 2,
        .h = simH * SIM_SCALE + 2
    });

    SDL_Color black = { .r = 0, .g = 0, .b = 0, .a = 255 };
    SDL_Color gray = { .r = 128, .g = 128, .b = 128, .a = 255 };

    drawShellText(0, black, "Generation: %d", generation + 1);
    drawShellText(1, black, "Step: %03d", step + 1);
    drawShellText(2, black, "Survival Rate: %.2f%%", survivalRate);

    if (generation > 0) {
        drawShellText(3, black, "Prev. Rate: %.2f%%", previousSurvivalRate);
    }

    drawShellText(12, gray, "Selection: %s", sim->selector.name);
    drawShellText(13, gray, "Seed: %d", sim->seed);
    drawShellText(14, gray, "Int. Neurons: %d", sim->maxInternalNeurons);
    drawShellText(15, gray, "No. of Genes: %d", sim->numberOfGenes);
    drawShellText(16, gray, "Mut. Rate: %.2f%%", sim->mutationRate * 100.0f);
    drawShellText(17, gray, "Gen. Pop.: %d", sim->population);
    drawShellText(18, gray, "Gen. Count: %d", sim->maxGenerations);

    // obstacles
    for (int i = 0; i < OBSTACLE_COUNT; i++) {
        Rect *r = &OBSTACLES[i];
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
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
            case SDLK_q:
                interrupted = true;
                break;
            case SDLK_SPACE:
                if (paused) {
                    simSendContinue();
                } else {
                    simSendPause();
                }
                paused = !paused;
                printf("Visualiser: Pause %s\n", paused ? "Enabled" : "Disabled");
                break;
            case SDLK_d:
                withDelay = !withDelay;
                printf("Visualiser: Frame Delay %s\n", withDelay ? "Enabled" : "Disabled");
                break;
            }
            break;
        }
    }
}

void visDrawStep(void)
{
    handleEvents();

    if (interrupted) return;

    if (drawableOrgsGenerationChanged || drawableOrgsStepChanged) {

        sem_wait(&drawableOrgsLock);

        for (int i = 0; i < sim->population; i++) {
            destroyOrganism(&drawableOrgsRead[i]);
            drawableOrgsRead[i] = copyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
        drawableOrgsGenerationChanged = false;
        drawableOrgsStepChanged = false;

        sem_post(&drawableOrgsLock);

        drawableOrgsReadablePopulated = true;
    }

    SDL_SetRenderTarget(renderer, fileTexture);

    visDrawShell();

    if (drawableOrgsReadablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            Organism *org = &drawableOrgsRead[i];

            if (org->alive) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }

            SDL_Point fullPoints[5] = {
                (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 1, .y = paddingTop + SIM_SCALE * org->pos.y},
                (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 1, .y = paddingTop + SIM_SCALE * org->pos.y + 1},
                (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 1, .y = paddingTop + SIM_SCALE * org->pos.y + 2},
                (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y + 1},
                (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y + 1},
            };
            SDL_RenderDrawPoints(renderer, fullPoints, 5);

            if (org->alive) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 32);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 32);
            }
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            fullPoints[0] = (SDL_Point) {
                .x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y
            };
            fullPoints[1] = (SDL_Point) {
                .x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y + 2
            };
            fullPoints[2] = (SDL_Point) {
                .x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y + 2
            };
            fullPoints[3] = (SDL_Point) {
                .x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y
            };
            SDL_RenderDrawPoints(renderer, fullPoints, 4);
        }
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
    SDL_FreeSurface(fileSurface);
    fileSurface = NULL;

    SDL_DestroyTexture(fileTexture);
    fileTexture = NULL;

    TTF_CloseFont(font);
    font = NULL;

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

void visSetObstacles(Rect *obstacles, int count)
{
    if (OBSTACLES != NULL) {
        free(OBSTACLES);
        OBSTACLES = NULL;
    }
    OBSTACLE_COUNT = count;
    if (count == 0)
        return;
    OBSTACLES = calloc(count, sizeof(Rect));
    memcpy(OBSTACLES, obstacles, sizeof(Rect) * count);
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

void visSendGeneration(Organism *orgs, int g)
{
    if (interrupted) return;

    sem_wait(&drawableOrgsLock);

    if (drawableOrgsWriteablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
    }

    for (int i = 0; i < sim->population; i++) {
        drawableOrgsWrite[i] = copyOrganism(&orgs[i]);
    }

    drawableOrgsStepChanged = true;
    drawableOrgsGenerationChanged = true;
    drawableOrgsWriteablePopulated = true;

    generation = g;
    step = 0;

    if (generation > 0) {
        previousSurvivalRate = survivalRate;
    }

    survivalRate = 100.0f;

    sem_post(&drawableOrgsLock);

    if (withDelay)
        simSendFramePause();
}

void visSendQuit(void)
{
    interrupted = true;
}

void visSendStep(Organism* orgs, int s)
{
    if (interrupted) return;
    // if (fastPlay && (s != sim->stepsPerGeneration - 1)) return;

    sem_wait(&drawableOrgsLock);

    if (drawableOrgsWriteablePopulated) {
        for (int i = 0; i < sim->population; i++) {
            // this time only change the data that can change during execution
            copyOrganismMutableState((Organism*)&drawableOrgsWrite[i], &orgs[i]);
        }
        drawableOrgsStepChanged = true;

        step = s;

        survivalRate = calculateSurvivalRate((Organism*)drawableOrgsWrite);
    }

    sem_post(&drawableOrgsLock);

    if (withDelay)
        simSendFramePause();
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

    visInit(sim->size.w, sim->size.h);

    simSendReady();
    sem_wait(&visualiserReadyLock);

    if (paused) {
        simSendPause();
    }

    while (!interrupted) {
        visDrawStep();

        if (withDelay) {
            SDL_Delay(1000 / FPS);
        }

        if (!paused && withDelay) {
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

#endif
