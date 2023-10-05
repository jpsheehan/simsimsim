#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "Simulator.h"
#include "Visualiser.h"

#define WIN_W 480
#define WIN_H 480
#define SIM_SCALE 3
#define FPS 60
#define SAVE_IMAGES false

#if SAVE_IMAGES
#include <SDL_image.h>
#endif

static SDL_Window *window;
static SDL_Renderer *renderer;
static uint32_t generation;
static uint32_t step;
static int seed;
static int paddingLeft, paddingTop, simW, simH;
static Rect *OBSTACLES = NULL;
static int OBSTACLE_COUNT = 0;
static bool playSteps = true;
static bool wantsToQuit = false;
static SDL_Texture* fileTexture = NULL;
static SDL_Surface* fileSurface = NULL;


void visDrawShell(void);
void visSetTitle(void);

void visSetSeed(int s)
{
  seed = s;
}

void visInit(uint32_t w, uint32_t h)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    fprintf(stderr, "Could not init SDL\n");
    exit(1);
  }

#if SAVE_IMAGES
  if (!IMG_Init(IMG_INIT_JPG)) {
    fprintf(stderr, "Coult not init img\n");
    exit(1);
  }
#endif

  window =
      SDL_CreateWindow("Visualiser", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
  if (window == NULL)
  {
    fprintf(stderr, "Could not create window\n");
    exit(1);
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL)
  {
    fprintf(stderr, "Could not create renderer\n");
    exit(1);
  }

  simW = w;
  simH = h;
  paddingLeft = (WIN_W - (SIM_SCALE * w)) / 2;
  paddingTop = (WIN_H - (SIM_SCALE * h)) / 2;

  visDrawShell();
  SDL_RenderPresent(renderer);

  fileTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, WIN_W, WIN_H);
  int width, height;
  SDL_QueryTexture(fileTexture, NULL, NULL, &width, &height);
  fileSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
}

void visDrawShell(void)
{
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &(SDL_Rect){.x = paddingLeft - 1,
                                           .y = paddingTop - 1,
                                           .w = simW * SIM_SCALE + 2,
                                           .h = simH * SIM_SCALE + 2});

  // draw obstacles
  for (int i = 0; i < OBSTACLE_COUNT; i++)
  {
    Rect *r = &OBSTACLES[i];
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderFillRect(renderer,
                       &(SDL_Rect){.x = paddingLeft + r->x * SIM_SCALE,
                                   .y = paddingTop + r->y * SIM_SCALE,
                                   .w = r->w * SIM_SCALE,
                                   .h = r->h * SIM_SCALE});
  }
}

void visSetGeneration(int g)
{
  generation = g;
  visSetTitle();
}

void visSetStep(int s)
{
  step = s;
}

void handleEvents()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
    case SDL_WINDOWEVENT:
      switch (e.window.type)
      {
      case SDL_WINDOWEVENT_CLOSE:
        wantsToQuit = true;
        break;
      }
      break;
    case SDL_KEYDOWN:
      switch (e.key.keysym.sym)
      {
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

void visDrawStep(Organism *orgs, uint32_t count, bool forceDraw)
{
  handleEvents();

  SDL_SetRenderTarget(renderer, fileTexture);

  if (!forceDraw && !playSteps)
  {
    return;
  }

  visSetTitle();
  visDrawShell();

  for (int i = 0; i < count; i++)
  {
    Organism *org = &orgs[i];

    if (org->alive)
    {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }
    else
    {
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
    
    if (org->alive)
    {
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 32);
    }
    else
    {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 32);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    fullPoints[0] = (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y};
    fullPoints[1] = (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x, .y = paddingTop + SIM_SCALE * org->pos.y + 2};
    fullPoints[2] = (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y + 2};
    fullPoints[3] = (SDL_Point){.x = paddingLeft + SIM_SCALE * org->pos.x + 2, .y = paddingTop + SIM_SCALE * org->pos.y};
    SDL_RenderDrawPoints(renderer, fullPoints, 4);
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

  // SDL_Delay(1000 / FPS);
}

void visDestroy(void)
{

    SDL_FreeSurface(fileSurface);
SDL_DestroyTexture(fileTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
#if SAVE_IMAGES
  IMG_Quit();
#endif
  SDL_Quit();
}

void visSetTitle(void)
{
  char title[100];
  snprintf(title, 100, "Gen %d, Step %d", generation, step);
  SDL_SetWindowTitle(window, title);
}

void visSetObstacles(Rect *obstacles, int count)
{
  if (OBSTACLES != NULL)
  {
    free(OBSTACLES);
    OBSTACLES = NULL;
  }
  OBSTACLE_COUNT = count;
  if (count == 0)
    return;
  OBSTACLES = calloc(count, sizeof(Rect));
  memcpy(OBSTACLES, obstacles, sizeof(Rect) * count);
}