#pragma once

#include "../runtime/KujakuApi.h"

namespace KujakuEngine {

class Scene;

/// <summary>
/// UIのポインタ入力処理。Canvasを走査してポインタ下の最前面Buttonを判定し、
/// ホバー/押下の見た目更新とクリック時のonClick発火を行う。描画パス外(Update)で呼ぶこと。
/// </summary>
KUJAKU_API void UpdateUIEventSystem(Scene& scene, float targetWidth, float targetHeight);

} // namespace KujakuEngine
