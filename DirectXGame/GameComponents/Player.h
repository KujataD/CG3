#pragma once
#include <KujakuEngine.h>
#include <components/AnimatorComponent.h>

class Player : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "Player"; }

	void Initialize() override;
	void OnPlayStart() override;
	void Update() override;

private:

	void Move();

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.05f, 0.0f, 100.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(turnSpeed_, "Turn Speed", 0.1f, 0.0f, 50.0f);
	}

	// 移動速度[unit/s]
	KUJAKU_FIELD_FLOAT(speed_, 5.0f);
	// 旋回速度[rad/s]
	KUJAKU_FIELD_FLOAT(turnSpeed_, 12.0f);

	KujakuEngine::AnimatorComponent* animator_ = nullptr;
};
