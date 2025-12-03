// undo.c
#include "undo.h"

#define MAX_HISTORY 20

// スタック
static SDL_Texture* undo_stack[MAX_HISTORY];
static SDL_Texture* redo_stack[MAX_HISTORY];
static int undo_top = -1;
static int redo_top = -1;

void save_undo(SDL_Renderer* renderer, SDL_Texture* canvas,
               int width, int height)
{
    if (undo_top < MAX_HISTORY - 1) {
        undo_top++;
        undo_stack[undo_top] = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_TARGET, width, height);

        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, undo_stack[undo_top]);
        SDL_RenderCopy(renderer, canvas, NULL, NULL);
        SDL_SetRenderTarget(renderer, prev);
    }
    // 新しい操作が入ったので Redo はクリア
    redo_top = -1;
}

void undo(SDL_Renderer* renderer, SDL_Texture* canvas,
          int width, int height)
{
    if (undo_top >= 0) {
        // 現在の状態を Redo スタックへ
        redo_top++;
        redo_stack[redo_top] = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_TARGET, width, height);

        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, redo_stack[redo_top]);
        SDL_RenderCopy(renderer, canvas, NULL, NULL);
        SDL_SetRenderTarget(renderer, prev);

        // Undo スタックから復元
        SDL_SetRenderTarget(renderer, canvas);
        SDL_RenderCopy(renderer, undo_stack[undo_top], NULL, NULL);
        SDL_SetRenderTarget(renderer, NULL);

        SDL_DestroyTexture(undo_stack[undo_top]);
        undo_top--;
    }
}

void redo(SDL_Renderer* renderer, SDL_Texture* canvas,
          int width, int height)
{
    if (redo_top >= 0) {
        // 現在の状態を Undo スタックへ
        undo_top++;
        undo_stack[undo_top] = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_TARGET, width, height);

        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, undo_stack[undo_top]);
        SDL_RenderCopy(renderer, canvas, NULL, NULL);
        SDL_SetRenderTarget(renderer, prev);

        // Redo スタックから復元
        SDL_SetRenderTarget(renderer, canvas);
        SDL_RenderCopy(renderer, redo_stack[redo_top], NULL, NULL);
        SDL_SetRenderTarget(renderer, NULL);

        SDL_DestroyTexture(redo_stack[redo_top]);
        redo_top--;
    }
}
