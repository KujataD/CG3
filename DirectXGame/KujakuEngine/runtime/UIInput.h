#pragma once

#include "KujakuApi.h"

namespace KujakuEngine {

/// <summary>
/// UIのポインタ入力状態。座標は「Game RT(ターゲット)ピクセル空間」(左上原点)。
/// エディタではGameViewWindowが、ランタイムでは主ループがセットする。
/// </summary>
struct UIPointerState {
	float x = 0.0f;
	float y = 0.0f;
	bool inside = false;   // ポインタがビュー内にあるか
	bool pressed = false;  // このフレームで押された(トリガ)
	bool held = false;     // 押下中
	bool released = false; // このフレームで離された
};

KUJAKU_API void SetUIPointer(const UIPointerState& state);
KUJAKU_API const UIPointerState& GetUIPointer();

} // namespace KujakuEngine
