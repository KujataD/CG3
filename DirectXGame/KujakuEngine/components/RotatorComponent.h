#pragma once

#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// 所有GameObjectを回転させるComponent
/// </summary>
class RotatorComponent : public Component {
public:
	const char* GetTypeName() const override { return "RotatorComponent"; }

	void Update() override;

	void SetSpeed(float speed) { speed_ = speed; }

	float GetSpeed() const { return speed_; }

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_FLOAT(speed_, 0.02f);
	float speed3_ = 0.02f;
};

} // namespace KujakuEngine
