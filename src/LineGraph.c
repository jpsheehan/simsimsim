#include "LineGraph.h"

LineGraph createLineGraph(size_t maxElements, SDL_Color lineColor, Pos pos, Size size)
{
    return (LineGraph){
        .count = 0,
        .lineColor = lineColor,
        .max = maxElements,
        .pos = pos,
        .size = size,
        .points = calloc(maxElements, sizeof(SDL_Point)),
        .values = calloc(maxElements, sizeof(float)),
    };
}

void destroyLineGraph(LineGraph* g)
{
    free(g->values);
    g->values = NULL;

    free(g->points);
    g->points = NULL;

    g->count = 0;
    g->max = 0;
}

void renderLineGraph(LineGraph* g, SDL_Renderer* r)
{
    // TODO: We can further speed this up by caching the texture and reusing it for the next draw.

    SDL_Rect border = { .x = g->pos.x, .y = g->pos.y, .w = g->size.w, .h = g->size.h };
    SDL_Color borderColor = { .r = 0, .g = 0, .b = 0, .a = 255 };

    // draw outline
    SDL_SetRenderDrawColor(r, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(r, &border);

    // draw 25%, 50%, and 75% lines
    SDL_SetRenderDrawColor(r, 128, 128, 128, 255);
    SDL_RenderDrawLine(r, g->pos.x, g->pos.y + g->size.h / 4, g->pos.x + g->size.w - 2, g->pos.y + g->size.h / 4);
    SDL_RenderDrawLine(r, g->pos.x, g->pos.y + g->size.h / 2, g->pos.x + g->size.w - 2, g->pos.y + g->size.h / 2);
    SDL_RenderDrawLine(r, g->pos.x, g->pos.y + 3 * g->size.h / 4, g->pos.x + g->size.w - 2, g->pos.y + 3 * g->size.h / 4);

    if (g->count >= 2) {
        SDL_SetRenderDrawColor(r, g->lineColor.r, g->lineColor.g, g->lineColor.b, g->lineColor.a);
        SDL_RenderDrawLines(r, g->points, g->count);
    }
}

void setPointOnGraph(LineGraph* g, size_t idx, float value)
{
    g->values[idx] = value;
    g->points[idx] = (SDL_Point){
        .x = g->pos.x + 1 + (int)((g->size.w - 2) * (float)idx / (float)g->max),
        .y = g->pos.y + 1 + g->size.h - 2 - (int)((float)(g->size.h - 2) * g->values[idx])
    };
    g->count = idx + 1;
}