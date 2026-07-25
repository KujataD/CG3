#pragma once

#include "../runtime/KujakuApi.h"

namespace KujakuEngine {

class Camera;
class Scene;

/// <summary>
/// world空間2Dスプライト(Sprite方式)の描画パス。
/// 3D描画の後・UI(Canvas)描画の前に呼ぶこと。
/// </summary>
KUJAKU_API void DrawSceneSprites(Scene& scene, Camera* camera);

} // namespace KujakuEngine
