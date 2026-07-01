#pragma once
#include <math/MathUtil.h>

namespace KujakuEngine {

/// <summary>
/// Particle構造体を表します。
/// </summary>
struct Particle {
	Vector3 translation;
	Vector3 velocity;
	Vector3 scale = {1.0f, 1.0f, 1.0f};
	Vector3 rotation;

	Vector4 color;
	float lifeTime = 1.0f;
	float currentTime = 0.0f;

	/// <summary>
	/// Deadかどうかを返します。
	/// </summary>
	bool IsDead() const { return currentTime >= lifeTime; }

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update(float deltaTime) {
		currentTime += deltaTime;
		translation += velocity * deltaTime;

		float t = currentTime / lifeTime;
		color.w = 1.0f - t;
	}
};

} // namespace KujakuEngine