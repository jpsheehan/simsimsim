#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL_ttf.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "Common.h"
#include "Organism.h"
#include "Simulator.h"
#include "Visualiser.h"

#define WIN_W 640
#define WIN_H 480
#define SIM_SCALE 3
#define FPS 30
#define SAVE_IMAGES false

#if SAVE_IMAGES
#include <SDL_image.h>
#endif

sem_t visualiserReadyLock;

#if FEATURE_VISUALISER

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
static Queue* inbox, *outbox;
static volatile bool interrupted = false;
static Simulation *sim;

static Organism* drawableOrgsRead;
static volatile Organism* drawableOrgsWrite;
static volatile bool drawableOrgsChanged;
static volatile bool drawableOrgsGenerationChanged;
static volatile bool drawableOrgsPopulated;
static bool drawableOrgsReadablePopulated;
static sem_t drawableOrgsLock;
static bool paused = true;
static bool fastPlay = false;

void visDrawShell(void);

void visInit(uint32_t w, uint32_t h)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Could not init SDL\n");
        exit(1);
    }

#if SAVE_IMAGES
    if (!IMG_Init(IMG_INIT_JPG)) {
        fprintf(stderr, "Could not init img\n");
        exit(1);
    }
#endif

    if (TTF_Init() == -1) {
        fprintf(stderr, "Could not ini ttf\n");
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

void visDrawShell(void)
{
    // printf("Begin drawing shell\n");

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

    // draw the text
    char buffer[128] = { 0 };
    SDL_Color black = { .r = 0, .g = 0, .b = 0, .a = 255 };
    SDL_Rect sourceRect, destRect;
    SDL_Surface* textSurface;
    SDL_Texture* textTexture;

    // Generation
    snprintf(buffer, 128, "Generation: %d", generation + 1);
    textSurface = TTF_RenderText_Blended(font, buffer, black);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    sourceRect = (SDL_Rect) {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };
    destRect = (SDL_Rect) {
        .x = paddingLeft * 1.5 + simW * SIM_SCALE, paddingTop, .w = textSurface->w, .h = textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, &sourceRect, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    // Step
    // printf("Finished renderering step: %d\n", step);
    snprintf(buffer, 128, "Step: %03d", step + 1);
    textSurface = TTF_RenderText_Blended(font, buffer, black);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    sourceRect = (SDL_Rect) {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };
    destRect = (SDL_Rect) {
        .x = paddingLeft * 1.5 + simW * SIM_SCALE, paddingTop + 20, .w = textSurface->w, .h = textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, &sourceRect, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    // Survival Rate
    snprintf(buffer, 128, "Survival Rate: %.2f%%", survivalRate);
    textSurface = TTF_RenderText_Blended(font, buffer, black);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    sourceRect = (SDL_Rect) {
        .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
    };
    destRect = (SDL_Rect) {
        .x = paddingLeft * 1.5 + simW * SIM_SCALE, paddingTop + 40, .w = textSurface->w, .h = textSurface->h
    };

    SDL_RenderCopy(renderer, textTexture, &sourceRect, &destRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    // Previous Gen. Survival Rate
    if (previousSurvivalRate >= 0.0f) {
        snprintf(buffer, 128, "Prev. Rate: %.2f%%", previousSurvivalRate);
        textSurface = TTF_RenderText_Blended(font, buffer, black);
        textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

        sourceRect = (SDL_Rect) {
            .x = 0, .y = 0, .w = textSurface->w, .h = textSurface->h
        };
        destRect = (SDL_Rect) {
            .x = paddingLeft * 1.5 + simW * SIM_SCALE, paddingTop + 60, .w = textSurface->w, .h = textSurface->h
        };

        SDL_RenderCopy(renderer, textTexture, &sourceRect, &destRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }

    // draw obstacles
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
    // printf("End drawing shell\n");
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

    if (drawableOrgsGenerationChanged && drawableOrgsReadablePopulated)
    {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism(&drawableOrgsRead[i]);
        }
    }

    if (drawableOrgsGenerationChanged || drawableOrgsChanged) {

        if (!drawableOrgsReadablePopulated)
        {
            drawableOrgsRead = calloc(sim->population, sizeof(Organism));
            drawableOrgsReadablePopulated = true;
        }

        sem_wait(&drawableOrgsLock);
        // printf("visDrawStep() #locked\n");

        for (int i = 0; i < sim->population; i++) {
            drawableOrgsRead[i] = copyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
        drawableOrgsGenerationChanged = false;
        drawableOrgsChanged = false;

        // printf("visDrawStep() #unlocked\n");
        sem_post(&drawableOrgsLock);

        drawableOrgsReadablePopulated = true;
    }

    // if (drawableOrgsChanged && drawableOrgsReadablePopulated)
    // {
    //     for (int i = 0; i < sim->population; i++) {
    //          drawableOrgsRead
    //     }
    // }

    if (drawableOrgsReadablePopulated) {
        int survivors = 0;
        for (int i = 0; i < sim->population; i++) {
            if (drawableOrgsRead[i].alive) {
                survivors++;
            }
        }
        survivalRate = 100.0f * (float)survivors / (float)sim->population;
    }

    SDL_SetRenderTarget(renderer, fileTexture);

    visDrawShell();

    if (drawableOrgsReadablePopulated)
    {
        // printf("Beginning render...\n");
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
        // printf("Ending render...\n");
    }

    SDL_RenderPresent(renderer);

#if SAVE_IMAGES
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
    SDL_DestroyTexture(fileTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
#if SAVE_IMAGES
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

void visSendGeneration(Organism *orgs, int g)
{
    if (interrupted) return;

    sem_wait(&drawableOrgsLock);

    if (drawableOrgsPopulated) {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
    }

    for (int i = 0; i < sim->population; i++) {
        drawableOrgsWrite[i] = copyOrganism(&orgs[i]);
    }

    drawableOrgsChanged = true;
    drawableOrgsPopulated = true;
    drawableOrgsGenerationChanged = true;

    generation = g;
    if (generation > 0) {
        previousSurvivalRate = survivalRate;
    }

    sem_post(&drawableOrgsLock);
    
    simSendFramePause();
}

void visSendQuit(void)
{
    interrupted = true;
}

void visSendStep(Organism* orgs, int s)
{
    if (interrupted) return;

    sem_wait(&drawableOrgsLock);

    if (drawableOrgsPopulated) {
        for (int i = 0; i < sim->population; i++) {
            // this time only change the data that can change during execution
            copyOrganismMutableState((Organism*)&drawableOrgsWrite[i], &orgs[i]);
        }
    }

    drawableOrgsChanged = true;
    
    step = s;

    sem_post(&drawableOrgsLock);
    
    simSendFramePause();
}

void runUserInterface(SharedThreadState* sharedThreadState)
{
    sim = sharedThreadState->sim;
    inbox = sharedThreadState->uiInbox;
    outbox = sharedThreadState->simInbox;

    drawableOrgsWrite = calloc(sim->population, sizeof(Organism));
    drawableOrgsRead = calloc(sim->population, sizeof(Organism));
    sem_init(&drawableOrgsLock, 0, 1);

    drawableOrgsChanged = false;
    drawableOrgsPopulated = false;
    drawableOrgsGenerationChanged = false;
    drawableOrgsReadablePopulated = false;

    visInit(sim->size.w, sim->size.h);

    simSendReady();
    sem_wait(&visualiserReadyLock);

    if (paused) {
        simSendPause();
    }

    while (!interrupted)
    {
        visDrawStep();

        SDL_Delay(1000 / FPS);

        if (!paused)
            simSendFrameContinue();
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

    if (drawableOrgsPopulated) {
        for (int i = 0; i < sim->population; i++) {
            destroyOrganism((Organism*)&drawableOrgsWrite[i]);
        }
        drawableOrgsPopulated = false;
        free((void*)drawableOrgsWrite);
        drawableOrgsWrite = NULL;
    }

    sem_destroy(&drawableOrgsLock);
}

#else

void visSendGeneration(Organism *orgs, int generation) {}
void visSendStep(Organism *orgs, int step) {}
void visSendQuit(void) {}

void runUserInterface(SharedThreadState *state)
{
    simSendReady();
    sem_wait(&visualiserReadyLock);
}

#endif

void visSendReady(void)
{
    sem_post(&visualiserReadyLock);
    // printf("visSendReady() #unlocked\n");
}
