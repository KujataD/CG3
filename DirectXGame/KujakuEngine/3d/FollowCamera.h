#pragma once
#include "Camera.h"
#include "WorldTransform.h"

namespace KujakuEngine {

class FollowCamera {
public:
	void Initialize();

	void Update();

	const Camera& GetCamera() const { return viewProjection_; }
	void SetTarget(const WorldTransform* target) { target_ = target; }

private:
	Camera viewProjection_;
	const WorldTransform* target_ = nullptr;
};

} // namespace KujakuEngine
