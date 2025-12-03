#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>

#include "ui.h"
#include "undo.h"

#define WINDOW_WIDTH 800    // window横幅
#define WINDOW_HEIGHT 600   // window縦幅
#define UI_HEIGHT 60        // 下部UI領域の高さ
#define SLIDER_Y (WINDOW_HEIGHT - UI_HEIGHT)

// 現在のツール状態（線の太さ・消しゴム）
typedef struct {
    int thickness;
    int eraser;
} ToolState;

// ユーティリティ
int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// 線プレビュー用の円描画
void draw_circle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w * w + h * h <= radius * radius)
                SDL_RenderDrawPoint(renderer, cx + w, cy + h);
        }
    }
}

// 描画内容の保存
void save_canvas(SDL_Renderer* renderer, SDL_Texture* canvas) {
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);

    SDL_SetRenderTarget(renderer, canvas);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888,
                         surf->pixels, surf->pitch);
    SDL_SaveBMP(surf, "output.bmp");
    SDL_FreeSurface(surf);
    SDL_Log(" Saved as output.bmp");
    SDL_SetRenderTarget(renderer, NULL);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow(
        "Draw App (Aligned Preview)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("font.ttf", 16);
    if (!font) {
        SDL_Log("Font load error: %s", TTF_GetError());
        return 1;
    }

    SDL_Texture* canvas = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
        WINDOW_WIDTH, SLIDER_Y);

    // 白背景で初期化
    SDL_SetRenderTarget(renderer, canvas);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

    SDL_Event e;
    int running = 1, drawing = 0, adjusting_slider = 0;
    int prevX = 0, prevY = 0, slider_pos = 200;
    int mouseX = 0, mouseY = 0;
    ToolState tool = {5, 0};   // 線の太さ5px、消しゴムOFF

    // UIボタン
    Button btn_clear  = {20,  SLIDER_Y + 25, 100, 30, "Clear",  0, 0};
    Button btn_eraser = {140, SLIDER_Y + 25, 100, 30, "Eraser", 0, 0};
    Button btn_save   = {260, SLIDER_Y + 25, 100, 30, "Save",   0, 0};

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = 0;

            // マウスボタン押下
            if (e.type == SDL_MOUSEBUTTONDOWN &&
                e.button.button == SDL_BUTTON_LEFT) {

                int mx = e.button.x, my = e.button.y;

                // ボタン処理
                if (is_button_clicked(btn_clear, mx, my)) {
                    SDL_SetRenderTarget(renderer, canvas);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderClear(renderer);
                    SDL_SetRenderTarget(renderer, NULL);
                    continue;
                }
                if (is_button_clicked(btn_eraser, mx, my)) {
                    tool.eraser = !tool.eraser;
                    btn_eraser.active = tool.eraser;
                    continue;
                }
                if (is_button_clicked(btn_save, mx, my)) {
                    save_canvas(renderer, canvas);
                    continue;
                }

                // スライダー
                if (my >= SLIDER_Y && my <= SLIDER_Y + 20) {
                    adjusting_slider = 1;
                    slider_pos = mx;
                    slider_pos = clamp(slider_pos, 20, WINDOW_WIDTH - 20);
                    tool.thickness = (slider_pos / 8) + 1;
                }
                // キャンバス
                else if (my < SLIDER_Y) {
                    drawing = 1;
                    prevX = mx;
                    prevY = my;
                    save_undo(renderer, canvas, WINDOW_WIDTH, SLIDER_Y);
                }
            }

            // マウスボタン離し
            if (e.type == SDL_MOUSEBUTTONUP &&
                e.button.button == SDL_BUTTON_LEFT) {
                drawing = 0;
                adjusting_slider = 0;
            }

            // マウス移動
            if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;

                if (adjusting_slider) {
                    slider_pos = e.motion.x;
                    slider_pos = clamp(slider_pos, 20, WINDOW_WIDTH - 20);
                    tool.thickness = (slider_pos / 8) + 1;
                }

                if (drawing && e.motion.y < SLIDER_Y) {
                    int x = e.motion.x, y = e.motion.y;
                    int dx = x - prevX, dy = y - prevY;
                    int steps = SDL_max(SDL_abs(dx), SDL_abs(dy));

                    SDL_SetRenderTarget(renderer, canvas);
                    if (tool.eraser)
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    else
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

                    for (int i = 0; i <= steps; i++) {
                        int px = prevX + dx * i / steps;
                        int py = prevY + dy * i / steps;
                        SDL_Rect rect = {
                            px - tool.thickness / 2,
                            py - tool.thickness / 2,
                            tool.thickness, tool.thickness
                        };
                        SDL_RenderFillRect(renderer, &rect);
                    }
                    SDL_SetRenderTarget(renderer, NULL);
                    prevX = x;
                    prevY = y;
                }
            }

            // キーボード（Ctrl+Z / Ctrl+Y）
            if (e.type == SDL_KEYDOWN) {
                if ((e.key.keysym.sym == SDLK_z) &&
                    (e.key.keysym.mod & KMOD_CTRL)) {
                    undo(renderer, canvas, WINDOW_WIDTH, SLIDER_Y);
                }
                if ((e.key.keysym.sym == SDLK_y) &&
                    (e.key.keysym.mod & KMOD_CTRL)) {
                    redo(renderer, canvas, WINDOW_WIDTH, SLIDER_Y);
                }
            }
        }

        // 画面描画
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        SDL_Rect canvas_dst = {0, 0, WINDOW_WIDTH, SLIDER_Y};
        SDL_RenderCopy(renderer, canvas, NULL, &canvas_dst);

        // ブラシプレビュー
        if (mouseY < SLIDER_Y) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 255, 150);
            draw_circle(renderer, mouseX, mouseY, tool.thickness / 2);
        }

        // 下部UI
        SDL_Rect ui_bg = {0, SLIDER_Y, WINDOW_WIDTH, UI_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderFillRect(renderer, &ui_bg);

        // スライダーラインとノブ
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
        SDL_RenderDrawLine(renderer, 20, SLIDER_Y + 10,
                           WINDOW_WIDTH - 20, SLIDER_Y + 10);
        SDL_Rect knob = {slider_pos - 5, SLIDER_Y, 10, 20};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &knob);

        // ボタン＆ラベル
        draw_button(renderer, btn_clear,  font);
        draw_button(renderer, btn_eraser, font);
        draw_button(renderer, btn_save,   font);
        draw_thickness_label(renderer, font, tool.thickness,
                             SLIDER_Y - 30);

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
