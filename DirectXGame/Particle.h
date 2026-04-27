#pragma once
#include <KujakuEngine.h>

class Particle {
public:

	static Particle MakeNewParticle(KujakuEngine::Camera* camera);

	void Initialize(KujakuEngine::Camera* camera);
	void Update();

	KujakuEngine::WorldTransform* GetWorldTransform() { return &transform_; }

private:
	KujakuEngine::WorldTransform transform_;
	KujakuEngine::Vector3 velocity_;
	KujakuEngine::Vector4 color_;
	KujakuEngine::Camera* camera_;
};
