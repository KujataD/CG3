#pragma once

#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// 所有GameObjectを回転させるComponent
/// </summary>
class RotatorComponent : public Component {
public:
	/// <summary>
	/// TypeNameを取得します。
	/// </summary>
	const char* GetTypeName() const override { return "RotatorComponent"; }

	/// <summary>
	/// 回転更新
	/// </summary>
	void Update() override;

	void SetSpeed(float speed) { speed_ = speed; }

	/// <summary>
	/// 回転速度を取得
	/// </summary>
	float GetSpeed() const { return speed_; }

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_FLOAT(speed_, 0.02f);
	float speed3_ = 0.02f;
};

} // namespace KujakuEngine
