#pragma once

#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../shapes/ShapeUtil.h"

namespace KujakuEngine {

class Camera;
class Component;
class GameObject;
class Scene;

/// <summary>
/// RayCastのヒット情報
/// </summary>
struct RayCastHit {
	GameObject* gameObject = nullptr;
	Component* renderer = nullptr;
	Vector3 point = {};
	float distance = 0.0f;
};

/// <summary>
/// Scene内の描画ObjectへRayを飛ばす機能
/// </summary>
class RayCast {
public:
	/// <summary>
	/// Viewport上の座標からCamera基準のRayを作成する
	/// </summary>
	static bool CreateRayFromViewportPoint(
	    const Vector2& point,
	    const Vector2& viewportPosition,
	    const Vector2& viewportSize,
	    const Camera& camera,
	    Ray& outRay);

	/// <summary>
	/// Scene内のRayCast対象ComponentにRayを当てる
	/// </summary>
	static bool Cast(const Scene& scene, const Ray& ray, RayCastHit& outHit);

	/// <summary>
	/// RayとAABBの交差判定
	/// </summary>
	static bool IntersectRayAabb(const Ray& ray, const Vector3& minValue, const Vector3& maxValue, float& outDistance);
};

} // namespace KujakuEngine
