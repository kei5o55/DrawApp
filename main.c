#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>
#include "config.h"


#include "ui.h"
#include "undo.h"
#include "slider.h"


typedef enum {//ブラシの種類
    BRUSH_RECT,
    BRUSH_CIRCLE,
    BRUSH_COUNT
} BrushType;

// 現在のツール状態（線の太さ・消しゴム）
typedef struct {
    int thickness;
    int eraser;
    BrushType brush;
} ToolState;

ToolState tool = {5, 0, BRUSH_CIRCLE};//線の太さ5px、消しゴムOFF、円形ブラシ（初期値

Button brushes[BRUSH_COUNT];//ブラシの種類ボタン配列



// ユーティリティ
int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// 円描画
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

    // UIボタン
    Button btn_clear  = {20,  SLIDER_Y + 25, 100, 30, "Clear",  0, 0};
    Button btn_eraser = {140, SLIDER_Y + 25, 100, 30, "Eraser", 0, 0};
    Button btn_save   = {260, SLIDER_Y + 25, 100, 30, "Save",   0, 0};

    // ブラシ選択ボタン（右下に配置）
    int brush_w = 80;//ボタン幅
    int brush_h = 30;//ボタン高さ
    int brush_y = SLIDER_Y + 25;//ボタンY位置
    int margin_right = 20;   // 右端からの余白
    int gap = 10;            // ボタン間の隙間

    // 右端から「Circle」「Rect」の順に並べるイメージ
    brushes[BRUSH_CIRCLE] = (Button){
        WINDOW_WIDTH - margin_right - brush_w,//X位置
        brush_y,
        brush_w, brush_h,
        "Circle",
        1, 0//初期選択状態
    };

    brushes[BRUSH_RECT] = (Button){
        WINDOW_WIDTH - margin_right - brush_w * 2 - gap,//X位置
        brush_y,
        brush_w, brush_h,
        "Rect",
        0, 0   
    };

    while (running) {//メインループ開始
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = 0;

            // マウスボタン押下
            if (e.type == SDL_MOUSEBUTTONDOWN &&
                e.button.button == SDL_BUTTON_LEFT) {

                int mx = e.button.x, my = e.button.y;//マウス座標(x,y)

                // ボタン処理
                if (is_button_clicked(btn_clear, mx, my)) {//clear
                    SDL_SetRenderTarget(renderer, canvas);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderClear(renderer);
                    SDL_SetRenderTarget(renderer, NULL);
                    continue;
                }
                if (is_button_clicked(btn_eraser, mx, my)) {//eraser
                    tool.eraser = !tool.eraser;//トグル切り替え
                    btn_eraser.active = tool.eraser;
                    continue;
                }
                if (is_button_clicked(btn_save, mx, my)) {//save
                    save_canvas(renderer, canvas);
                    continue;
                }
                for (int i = 0; i < BRUSH_COUNT; i++) {
                     if (is_button_clicked(brushes[i], mx, my)) {
                        tool.brush = i;
                    for (int j = 0; j < BRUSH_COUNT; j++)
                        brushes[j].active = (j == i);
                        break;
                    }
                }


                // スライダー
                if (my >= SLIDER_Y && my <= SLIDER_Y + 20 &&mx >= 20 && mx <= WINDOW_WIDTH - 20) {
                    adjusting_slider = 1;//スライダーを掴んでいるフラグ
                    slider_pos = mx;//スライダーの位置更新
                    slider_pos = clamp(slider_pos, 20, WINDOW_WIDTH - 20);//範囲
                    slider_apply(mx, &slider_pos, &tool.thickness);/*サイズ変更の関数を呼び出し（マウスX座標、スライダー位置、線の太さ）
                                                                       メインの変数を変更したいのでポインタで渡す．mxはただの値なのでそのまま*/
                    continue;

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
                    slider_pos = mouseX;//スライダーの位置更新
                    slider_pos = clamp(slider_pos, 20, WINDOW_WIDTH - 20);//範囲

                    slider_apply(mouseX, &slider_pos, &tool.thickness);/*サイズ変更の関数を呼び出し（マウスX座標、スライダー位置、線の太さ）
                                                                       メインの変数を変更したいのでポインタで渡す．mxはただの値なのでそのまま*/
                    char text[64];
                    sprintf(text, "Knob X: %d", slider_pos);//デバッグ用
                    draw_text(renderer, font, text, 100, SLIDER_Y - 50);
                    printf("Slider position: %d\n", slider_pos);

                }

                if (drawing && e.motion.y < SLIDER_Y) {//描画モードかつキャンバス内
                    
                    int x = e.motion.x, y = e.motion.y;//現在座標
                    int dx = x - prevX, dy = y - prevY;//移動量の差分を取る
                    int steps = SDL_max(SDL_abs(dx), SDL_abs(dy));//分割数計算ピクセル単位で計算しているので拡張性あり

                    SDL_SetRenderTarget(renderer, canvas);//レンダリングターゲットをキャンバスに変更
                    if (tool.eraser)//消しゴムモードのとき
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);//白色で描画
                    else
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);//黒色で描画（ここに色選択機能を追加しても良い）

                    for (int i = 0; i <= steps; i++) {//補完描画ループ（steps=分割数）
                        int px = prevX + dx * i / steps;//補完座標計算（floatにするとより正確な補完？
                        int py = prevY + dy * i / steps;
                        int radius = tool.thickness / 2;//半径（ツールサイズの半分
                        switch (tool.brush) {
                            case BRUSH_RECT:   SDL_Rect rect = { px - radius,py - radius,tool.thickness,tool.thickness};//四角形描写（ここでしか使わないので円と違い直接描いています）
                                                SDL_RenderFillRect(renderer, &rect);
                                                break;
                            case BRUSH_CIRCLE: draw_circle(renderer, px, py, radius); break;//円描写（関数から）
                            }

                    }
                    SDL_SetRenderTarget(renderer, NULL);//レンダリングターゲットを元に戻す
                    prevX = x;//前回座標更新
                    prevY = y;//前回座標更新
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
        for (int i = 0; i < BRUSH_COUNT; i++) draw_button(renderer, brushes[i], font);


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

