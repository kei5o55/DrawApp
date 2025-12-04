// slider.h
#pragma once

// スライダー上をクリックしたか？
int slider_hit(int mx, int my);

// マウスのX座標から、つまみ位置とブラシ太さを更新
void slider_apply(int mx, int* slider_pos, int* thickness);
