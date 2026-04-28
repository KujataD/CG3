#include "ParticleEmitter.h"
#include "../shapes/ShapeUtil.h"

namespace KujakuEngine {

using namespace ShapeUtil;

void ParticleEmitter::Initialize(ParticleModel* model) { model_ = model; }

void ParticleEmitter::Update(float deltaTime, const Camera& camera) {
	frequencyTime_ += deltaTime;        // 時刻を進める
	if (frequency_ <= frequencyTime_) { // 頻度より大きいなら発生
		Emit();                         // パーティクル生成処理
		frequencyTime_ -= frequency_;   // 余計に過ぎた時間も加味して頻度計算する
	}

	for (std::list<Particle>::iterator particleIterator = particles_.begin(); particleIterator != particles_.end();) {
		// ライフタイムを進める
		(*particleIterator).currentTime += deltaTime;

		// すでに死んでいたら、リストから削除
		if ((*particleIterator).IsDead()) {
			particleIterator = particles_.erase(particleIterator);
			continue;
		}

		// Field計算処理
		if (isActiveField_) {
			for (auto& field : accelerationFields_) {
				if (IsCollision(field.area, (*particleIterator).translation)) {
					(*particleIterator).velocity += field.acceleration * deltaTime;
				}
			}
		}

		// 透明度計算
		float alpha = 1.0f - ((*particleIterator).currentTime / (*particleIterator).lifeTime);
		(*particleIterator).color.w = alpha;
		(*particleIterator).translation += (*particleIterator).velocity * deltaTime;

		TransformationMatrix billboardMatrix = Matrix4x4::MakeBillboardMatrix((*particleIterator).scale, (*particleIterator).rotation, (*particleIterator).translation, camera);

		// InstancingModelに追加
		model_->AddInstanceParticle(billboardMatrix, (*particleIterator).color);
		++particleIterator;
	}
	model_->UpdateBuffer();
}

void ParticleEmitter::Draw() {
	if (!model_) {
		return;
	}

	model_->Draw();
}

void ParticleEmitter::Emit() {
	for (uint32_t count = 0; count < count_; ++count) {
		particles_.push_back(MakeParticle());
	}
}

Particle ParticleEmitter::MakeParticle() {
	Particle particle;
	Vector3 randomTranslation = {Random::GetRandom(-1.0f * scale_.x, 1.0f * scale_.x), Random::GetRandom(-1.0f * scale_.y, 1.0f * scale_.y), Random::GetRandom(-1.0f * scale_.z, 1.0f * scale_.z)};

	particle.translation = translation_ + randomTranslation;
	particle.velocity = {Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f)};
	particle.color = {Random::GetRandom(0.0f, 1.0f), Random::GetRandom(0.0f, 1.0f), Random::GetRandom(0.0f, 1.0f), 1.0f};
	particle.lifeTime = Random::GetRandom(1.0f, 3.0f);
	particle.currentTime = 0.0f;
	return particle;
}

} // namespace KujakuEngine