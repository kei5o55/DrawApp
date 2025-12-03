// ui.h
#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// ボタン情報
typedef struct {
    int x, y, w, h;
    char label[16];
    int active;
    int pressed;
} Button;

// テキスト描画
void draw_text(SDL_Renderer* renderer, TTF_Font* font,
               const char* text, int x, int y);

// ボタン描画
void draw_button(SDL_Renderer* r, Button btn, TTF_Font* font);

// ボタンがクリックされたか
int is_button_clicked(Button btn, int mx, int my);

// 線の太さラベル（y位置を引数で渡すようにした）
void draw_thickness_label(SDL_Renderer* renderer, TTF_Font* font,
                          int thickness, int y);
