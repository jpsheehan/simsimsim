#include <stdlib.h>
#include <stdio.h>

#include "sim.h"
#include "visualiser.h"

#define WIN_W 320
#define WIN_H 320
#define SIM_SCALE 2

static SDL_Window* window;
static SDL_Renderer* renderer;
static uint32_t generation;
static uint32_t step;
static int paddingLeft, paddingTop, simW, simH;

void visDrawShell(void);

void visInit(uint32_t w, uint32_t h)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Could not init SDL\n");
        exit(1);
    }

    window = SDL_CreateWindow("Visualiser", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Could not create renderer\n");
        exit(1);
    }

    simW = w;
    simH = h;
    paddingLeft = (WIN_W - (SIM_SCALE * w)) / 2;
    paddingTop = (WIN_H - (SIM_SCALE * h)) / 2;

    visDrawShell();
    SDL_RenderPresent(renderer);
}

void visDrawShell(void)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &(SDL_Rect){ .x = paddingLeft, .y = paddingTop, .w = simW * SIM_SCALE, .h = simH * SIM_SCALE});
}

void visSetGeneration(int g)
{
    generation = g;
}

void visSetStep(int s)
{
    step = s;
}

void visDrawStep(Organism* orgs, uint32_t count)
{
    visDrawShell();

    for (int i = 0; i < count; i++)
    {
        Organism* org = &orgs[i];

        if (org->alive) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        }

        SDL_RenderFillRect(renderer, &(SDL_Rect){
            .x = paddingLeft + SIM_SCALE * org->pos.x,
            .y = paddingTop  + SIM_SCALE * org->pos.y,
            .w = SIM_SCALE,
            .h = SIM_SCALE
        });
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(1000 / 30);
}

void visDestroy(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}