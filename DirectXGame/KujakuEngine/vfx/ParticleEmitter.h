#pragma once
#include "../math/Matrix4x4.h"
#include "../math/Vector3.h"
#include "../math/Random.h"
#include "Particle.h"
#include "ParticleModel.h"
#include "ParticleField.h"

#include <list>
#include <vector>

namespace KujakuEngine {

class ParticleEmitter {
public:
	void Initialize(ParticleModel* model);

	void Update(float deltaTime, const Camera& camera);
	void Draw();

	void Emit();

	void AddField(AccelerationField& field) { accelerationFields_.push_back(field); }
	void SetIsActiveField(bool isActive) { isActiveField_ = isActive; }

private:
	Particle MakeParticle();

public:
	Vector3 translation_;
	Vector3 rotation_;
	Vector3 scale_ = {1.0f, 1.0f, 1.0f};

	uint32_t count_ = 3; // !< 発生数
	float frequency_ = 0.5f;     // !< 発生頻度
	float frequencyTime_ = 0.0f; // !< 頻度用時刻

private:
	// パーティクルモデル
	ParticleModel* model_ = nullptr;

	std::list<Particle> particles_;

	// フィールド
	std::list<AccelerationField> accelerationFields_;
	bool isActiveField_ = true;
};

} // namespace KujakuEngine
