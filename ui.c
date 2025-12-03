// ui.c
#include "ui.h"
#include <stdio.h>


// テキスト描画
void draw_text(SDL_Renderer* renderer, TTF_Font* font,
               const char* text, int x, int y)
{
    SDL_Color color = {0, 0, 0, 255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    int tw, th;
    SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
    SDL_Rect dst = {x - tw / 2, y - th / 2, tw, th};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

// ボタン描画
void draw_button(SDL_Renderer* r, Button btn, TTF_Font* font)
{
    if (btn.pressed)
        SDL_SetRenderDrawColor(r, 160, 200, 255, 255);
    else if (btn.active)
        SDL_SetRenderDrawColor(r, 180, 220, 255, 255);
    else
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);

    SDL_Rect rect = {btn.x, btn.y, btn.w, btn.h};
    SDL_RenderFillRect(r, &rect);

    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
    SDL_RenderDrawRect(r, &rect);

    draw_text(r, font, btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2);
}

// ボタンクリック判定
int is_button_clicked(Button btn, int mx, int my)
{
    return (mx >= btn.x && mx <= btn.x + btn.w &&
            my >= btn.y && my <= btn.y + btn.h);
}

// 線の太さラベル
void draw_thickness_label(SDL_Renderer* renderer, TTF_Font* font,
                          int thickness, int y)
{
    char text[32];
    snprintf(text, sizeof(text), "Thickness: %d px", thickness);
    SDL_Color color = {0, 0, 0, 255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = {120, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}
