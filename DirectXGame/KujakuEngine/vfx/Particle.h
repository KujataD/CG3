#pragma once
#include "../math/Vector3.h"
#include "../math/Vector4.h"

namespace KujakuEngine {

struct Particle {
	Vector3 translation;
	Vector3 velocity;
	Vector3 scale = {1.0f, 1.0f, 1.0f};
	Vector3 rotation;

	Vector4 color;
	float lifeTime = 1.0f;
	float currentTime = 0.0f;

	bool IsDead() const { return currentTime >= lifeTime; }

	void Update(float deltaTime) {
		currentTime += deltaTime;
		translation += velocity * deltaTime;

		float t = currentTime / lifeTime;
		color.w = 1.0f - t;
	}
};

} // namespace KujakuEngine