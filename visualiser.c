#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim.h"
#include "visualiser.h"

#define WIN_W 480
#define WIN_H 480
#define SIM_SCALE 3
#define FPS 60

static SDL_Window *window;
static SDL_Renderer *renderer;
static uint32_t generation;
static uint32_t step;
static int paddingLeft, paddingTop, simW, simH;
static Rect *OBSTACLES = NULL;
static int OBSTACLE_COUNT = 0;
static bool playSteps = true;
static bool wantsToQuit = false;

void visDrawShell(void);
void visSetTitle(void);

void visInit(uint32_t w, uint32_t h) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "Could not init SDL\n");
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

  simW = w;
  simH = h;
  paddingLeft = (WIN_W - (SIM_SCALE * w)) / 2;
  paddingTop = (WIN_H - (SIM_SCALE * h)) / 2;

  visDrawShell();
  SDL_RenderPresent(renderer);
}

void visDrawShell(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &(SDL_Rect){.x = paddingLeft - 1,
                                           .y = paddingTop - 1,
                                           .w = simW * SIM_SCALE + 2,
                                           .h = simH * SIM_SCALE + 2});

  // draw obstacles
  for (int i = 0; i < OBSTACLE_COUNT; i++) {
    Rect *r = &OBSTACLES[i];
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderFillRect(renderer,
                       &(SDL_Rect){.x = paddingLeft + r->x * SIM_SCALE,
                                   .y = paddingTop + r->y * SIM_SCALE,
                                   .w = r->w * SIM_SCALE,
                                   .h = r->h * SIM_SCALE});
  }
}

void visSetGeneration(int g) {
  generation = g;
  visSetTitle();
}

void visSetStep(int s) {
  step = s;
  visSetTitle();
}

void handleEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_WINDOWEVENT:
      switch (e.window.type) {
      case SDL_WINDOWEVENT_CLOSE:
        wantsToQuit = true;
        break;
      }
      break;
    case SDL_KEYDOWN:
      switch (e.key.keysym.sym) {
      case SDLK_ESCAPE:
        wantsToQuit = true;
        break;
      case SDLK_q:
        wantsToQuit = true;
        break;
      case SDLK_SPACE:
        playSteps = !playSteps;
        break;
      }
      break;
    }
  }
}

bool visGetWantsToQuit(void) { return wantsToQuit; }

void visDrawStep(Organism *orgs, uint32_t count, bool forceDraw) {
  handleEvents();

  if (!forceDraw && !playSteps) {
    return;
  }

  visDrawShell();

  for (int i = 0; i < count; i++) {
    Organism *org = &orgs[i];

    if (org->alive) {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    }

    SDL_RenderFillRect(renderer,
                       &(SDL_Rect){.x = paddingLeft + SIM_SCALE * org->pos.x,
                                   .y = paddingTop + SIM_SCALE * org->pos.y,
                                   .w = SIM_SCALE,
                                   .h = SIM_SCALE});
  }

  SDL_RenderPresent(renderer);
  SDL_Delay(1000 / FPS);
}

void visDestroy(void) {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void visSetTitle(void) {
  char title[100];
  snprintf(title, 100, "Gen %d, Step %d", generation, step);
  SDL_SetWindowTitle(window, title);
}

void visSetObstacles(Rect *obstacles, int count) {
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