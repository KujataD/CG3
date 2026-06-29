#pragma once

#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// 所有GameObjectを回転させるComponent
/// </summary>
class RotatorComponent : public Component {
public:
	const char* GetTypeName() const override { return "RotatorComponent"; }

	/// <summary>
	/// 回転更新
	/// </summary>
	void Update() override;

	/// <summary>
	/// Inspector表示
	/// </summary>
	void DrawInspector() override;

	/// <summary>
	/// Component情報をJSON形式で書き出す
	/// </summary>
	void WriteJson(std::ostream& os, int indent) const override;

	/// <summary>
	/// 回転速度を設定
	/// </summary>
	void SetSpeed(float speed) { speed_ = speed; }

	/// <summary>
	/// 回転速度を取得
	/// </summary>
	float GetSpeed() const { return speed_; }

private:
	float speed_ = 0.02f;
	float speed3_ = 0.02f;
};

} // namespace KujakuEngine
