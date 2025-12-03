// undo.h
#pragma once
#include <SDL2/SDL.h>

// width = WINDOW_WIDTH, height = SLIDER_Y を渡す
void save_undo(SDL_Renderer* renderer, SDL_Texture* canvas,
               int width, int height);
void undo(SDL_Renderer* renderer, SDL_Texture* canvas,
          int width, int height);
void redo(SDL_Renderer* renderer, SDL_Texture* canvas,
          int width, int height);
