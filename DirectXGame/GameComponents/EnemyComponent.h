#pragma once

#include <KujakuEngine.h>

/// <summary>
/// Enemy用のゲーム固有Component。
/// </summary>
class EnemyComponent : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "EnemyComponent"; }

	void Update() override;

	void SetSpeed(float speed) { speed_ = speed; }

	float GetSpeed() const { return speed_; }

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_FLOAT(speed_, 0.02f);
};
