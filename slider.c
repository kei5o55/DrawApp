// slider.c
#include <SDL2/SDL.h>
#include "config.h"
#include "slider.h"

// slider.c 内だけで使う clamp
static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// スライダー上をクリックしているかの判定
int slider_hit(int mx, int my) {
    // 縦方向: スライダーの高さ 20px
    // 横方向: 左右 20px ずつ余白
    if (my >= SLIDER_Y && my <= SLIDER_Y + 20 &&
        mx >= 20        && mx <= WINDOW_WIDTH - 20) {
        return 1;
    }
    return 0;
}

// マウスの X 座標から slider_pos と thickness を更新
void slider_apply(int mx, int* slider_pos, int* thickness) {
    // つまみの中心位置を更新（レール内に clamp）
    *slider_pos = clamp_int(mx, 20, WINDOW_WIDTH - 20);

    // 0.0 ～ 1.0 に正規化
    float t = (float)(*slider_pos - 20) / (WINDOW_WIDTH - 40);

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // 1 ～ 50px に変換
    *thickness = (int)(1 + t * 49);
}
