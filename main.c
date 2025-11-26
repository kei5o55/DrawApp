
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 800    //window横幅
#define WINDOW_HEIGHT 600   //window縦幅
#define UI_HEIGHT 60    //下部UI領域の高さ
#define SLIDER_Y (WINDOW_HEIGHT - UI_HEIGHT)
#define MAX_HISTORY 20  //Undo/Redo履歴の最大数

// ===== 構造体定義 =====
//ボタン情報をまとめる構造体
typedef struct {
    int x, y, w, h;            // 位置とサイズ
    char label[16];            // ボタンラベル
    int active;                // 状態フラグ（ON/OFF）
    int pressed;               // 押下中フラグ
} Button;

// 現在のツール状態（線の太さ・消しゴム）
typedef struct {
    int thickness;
    int eraser;
} ToolState;

// ===== Undo / Redo 用のスタック =====
SDL_Texture* undo_stack[MAX_HISTORY];
SDL_Texture* redo_stack[MAX_HISTORY];
int undo_top = -1;
int redo_top = -1;

// ===== ユーティリティ関数 =====
int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ===== テキスト描画関数 =====
void draw_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
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

// ===== ボタン描画関数 =====
void draw_button(SDL_Renderer* r, Button btn, TTF_Font* font) {
    //状態に応じた色を設定
    if (btn.pressed)
        SDL_SetRenderDrawColor(r, 160, 200, 255, 255);
    else if (btn.active)
        SDL_SetRenderDrawColor(r, 180, 220, 255, 255);
    else
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);

    //ボタン矩形描画
    SDL_Rect rect = {btn.x, btn.y, btn.w, btn.h};
    SDL_RenderFillRect(r, &rect);

    //枠線
    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
    SDL_RenderDrawRect(r, &rect);

    //中央にラベル文字を描画
    draw_text(r, font, btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2);
}

//ボタンクリック判定
int is_button_clicked(Button btn, int mx, int my) {
    return (mx >= btn.x && mx <= btn.x + btn.w &&
            my >= btn.y && my <= btn.y + btn.h);
}

// ===== 線の太さをUI上に表示 =====
void draw_thickness_label(SDL_Renderer* renderer, TTF_Font* font, int thickness) {
    char text[32];
    snprintf(text, sizeof(text), "Thickness: %d px", thickness);
    SDL_Color color = {0, 0, 0, 255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = {120, SLIDER_Y - 30, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

// ===== 線プレビュー用の円描画 =====
void draw_circle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int w = -radius; w <= radius; w++) {
        for (int h = -radius; h <= radius; h++) {
            if (w * w + h * h <= radius * radius)
                SDL_RenderDrawPoint(renderer, cx + w, cy + h);
        }
    }
}

// ===== 描画内容の保存機能 =====
void save_canvas(SDL_Renderer* renderer, SDL_Texture* canvas) {
    int width, height;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);

    //canvasの内容を取得してBMPに保存
    SDL_SetRenderTarget(renderer, canvas);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surf->pixels, surf->pitch);
    SDL_SaveBMP(surf, "output.bmp");
    SDL_FreeSurface(surf);
    SDL_Log(" Saved as output.bmp");
    SDL_SetRenderTarget(renderer, NULL);
}

// ===== Undo / Redo 機能 =====
void save_undo(SDL_Renderer* renderer, SDL_Texture* canvas) {
    if (undo_top < MAX_HISTORY - 1) {
        undo_top++;
        undo_stack[undo_top] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, SLIDER_Y);
        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, undo_stack[undo_top]);
        SDL_RenderCopy(renderer, canvas, NULL, NULL);
        SDL_SetRenderTarget(renderer, prev);
    }
    redo_top = -1; //Redo履歴をリセット
}

void undo(SDL_Renderer* renderer, SDL_Texture* canvas) {
    if (undo_top >= 0) {
        //現在の状態をRedoスタックに退避
        redo_top++;
        redo_stack[redo_top] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, SLIDER_Y);
        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, redo_stack[redo_top]);
        SDL_RenderCopy(renderer, canvas, NULL, NULL);
        SDL_SetRenderTarget(renderer, prev);

        //Undo履歴から復元
        SDL_SetRenderTarget(renderer, canvas);
        SDL_RenderCopy(renderer, undo_stack[undo_top], NULL, NULL);
        SDL_SetRenderTarget(renderer, NULL);

        SDL_DestroyTexture(undo_stack[undo_top]);
        undo_top--;
    }
}

void redo(SDL_Renderer* renderer, SDL_Texture* canvas) {
    if (redo_top >= 0) {
        undo_top++;
        undo_stack[undo_top] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, SLIDER_Y);
        SDL_Texture* prev = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, undo_stack[undo_top]);
        SDL_RenderCopy(renderer, canvas, NULL, NULL);
        SDL_SetRenderTarget(renderer, prev);

        SDL_SetRenderTarget(renderer, canvas);
        SDL_RenderCopy(renderer, redo_stack[redo_top], NULL, NULL);
        SDL_SetRenderTarget(renderer, NULL);

        SDL_DestroyTexture(redo_stack[redo_top]);
        redo_top--;
    }
}

// ===== メイン関数 =====
int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    // --- ウィンドウとレンダラーの初期化 ---
    SDL_Window* window = SDL_CreateWindow("Draw App (Aligned Preview)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // --- フォント読み込み ---
    TTF_Font* font = TTF_OpenFont("font.ttf", 16);
    if (!font) {
        SDL_Log("Font load error: %s", TTF_GetError());
        return 1;
    }

    // --- 描画キャンバスの作成 ---
    SDL_Texture* canvas = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_TARGET,
        WINDOW_WIDTH, SLIDER_Y);

    //白背景で初期化
    SDL_SetRenderTarget(renderer, canvas);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

    // --- 初期状態変数 ---
    SDL_Event e;
    int running = 1, drawing = 0, adjusting_slider = 0;
    int prevX = 0, prevY = 0, slider_pos = 200;
    int mouseX = 0, mouseY = 0;
    ToolState tool = {5, 0};   //線の太さ5px、消しゴムOFF
    int previewX = -1, previewY = -1;
    int show_preview = 0;

    // --- UIボタン定義 ---
    Button btn_clear = {20, SLIDER_Y + 25, 100, 30, "Clear", 0, 0};
    Button btn_eraser = {140, SLIDER_Y + 25, 100, 30, "Eraser", 0, 0};
    Button btn_save = {260, SLIDER_Y + 25, 100, 30, "Save", 0, 0};

    // ===== メインループ =====
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = 0;

            // --- マウス入力処理 ---
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;//クリック座標を保存

                // 各ボタン処理
                if (is_button_clicked(btn_clear, mx, my)) {//clear(画面初期化)が押されたとき
                    SDL_SetRenderTarget(renderer, canvas);//rendererの描画対象をcanvasに
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);//背景色を白に設定
                    SDL_RenderClear(renderer);//初期化
                    SDL_SetRenderTarget(renderer, NULL);
                    show_preview = 0;
                    continue;
                }
                if (is_button_clicked(btn_eraser, mx, my)) {//eraser(消しゴム)
                    tool.eraser = !tool.eraser;//消しゴムのオンオフを切り替え
                    btn_eraser.active = tool.eraser;//ボタンのアクティブフラグを切り替え（ボタンの見た目が変わるように）
                    continue;
                }
                if (is_button_clicked(btn_save, mx, my)) {//save
                    save_canvas(renderer, canvas);//現在のcanvasをbmpファイルとして保存
                    continue;
                }

                // スライダー操作判定
                if (my >= SLIDER_Y && my <= SLIDER_Y + 20) {//クリック位置がスライダーの範囲内か
                    adjusting_slider = 1;//スライダーを掴んだ状態であることを示すフラグ(269行目)
                    slider_pos = mx;//スライダーのつまみ位置をクリック位置に設定
                    tool.thickness = (slider_pos / 8) + 1;//スライダーの位置から線の太さを更新
                } else if (my < SLIDER_Y) {//クリック位置がキャンバス上
                    // キャンバス上での描画開始
                    drawing = 1;//マウスドラッグ中の描画モードのオン
                    prevX = mx;//クリック時の位置
                    prevY = my;
                    save_undo(renderer, canvas); // Undo履歴保存
                    previewX = mx;//ブラシのプレビュー用位置
                    previewY = my;
                    show_preview = 1;//プレビュー表示をオン
                }
            }

            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                drawing = 0;
                adjusting_slider = 0;
            }

            // --- マウス移動時の処理 ---
            if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                if (adjusting_slider) {//マウスがつまみを掴んでいる状態のとき
                    slider_pos = e.motion.x;//つまみ位置をマウスの位置に更新
                    tool.thickness = (slider_pos / 8) + 1;//線の太さを更新
                }
                if (drawing && e.motion.y < SLIDER_Y) {
                    // 線の補間描画（ブレ防止）
                    int x = e.motion.x, y = e.motion.y;
                    int dx = x - prevX, dy = y - prevY;
                    int steps = SDL_max(SDL_abs(dx), SDL_abs(dy));

                    SDL_SetRenderTarget(renderer, canvas);
                    if (tool.eraser)
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    else
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

                    // ドット間を矩形で埋める
                    for (int i = 0; i <= steps; i++) {
                        int px = prevX + dx * i / steps;
                        int py = prevY + dy * i / steps;
                        SDL_Rect rect = {px - tool.thickness / 2, py - tool.thickness / 2,
                                         tool.thickness, tool.thickness};
                        SDL_RenderFillRect(renderer, &rect);
                    }
                    SDL_SetRenderTarget(renderer, NULL);
                    prevX = x;
                    prevY = y;
                }
            }

            // --- キーボード入力（Ctrl+Z / Ctrl+Y） ---
            if (e.type == SDL_KEYDOWN) {
                if ((e.key.keysym.sym == SDLK_z) && (e.key.keysym.mod & KMOD_CTRL))
                    undo(renderer, canvas);
                if ((e.key.keysym.sym == SDLK_y) && (e.key.keysym.mod & KMOD_CTRL))
                    redo(renderer, canvas);
            }
        }

        // ===== 画面描画処理 =====
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // --- キャンバスの描画 ---
        SDL_Rect canvas_dst = {0, 0, WINDOW_WIDTH, SLIDER_Y};
        SDL_RenderCopy(renderer, canvas, NULL, &canvas_dst);

        // --- マウス位置にブラシ範囲をプレビュー ---
        if (mouseY < SLIDER_Y) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 255, 150);
            draw_circle(renderer, mouseX, mouseY, tool.thickness / 2);
        }

        // --- 下部UI描画 ---
        SDL_Rect ui_bg = {0, SLIDER_Y, WINDOW_WIDTH, UI_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderFillRect(renderer, &ui_bg);

        // スライダーラインとノブ
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
        SDL_RenderDrawLine(renderer, 20, SLIDER_Y + 10, WINDOW_WIDTH - 20, SLIDER_Y + 10);
        SDL_Rect knob = {slider_pos - 5, SLIDER_Y, 10, 20};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &knob);

        // ボタンとラベル描画
        draw_button(renderer, btn_clear, font);
        draw_button(renderer, btn_eraser, font);
        draw_button(renderer, btn_save, font);
        draw_thickness_label(renderer, font, tool.thickness);

        SDL_RenderPresent(renderer);
    }

    // --- 終了処理 ---
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyTexture(canvas);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}