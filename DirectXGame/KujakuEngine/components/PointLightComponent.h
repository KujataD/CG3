#pragma once

#include "../3d/PointLight.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// GameObjectのTransform位置をPointLightへ反映するComponent
/// </summary>
class PointLightComponent : public Component {
public:
	/// <summary>
	/// TypeNameを取得します。
	/// </summary>
	const char* GetTypeName() const override { return "PointLightComponent"; }
	/// <summary>
	/// AllowMultipleを実行します。
	/// </summary>
	bool AllowMultiple() const override { return false; }
	/// <summary>
	/// EditorBillboardを持つかどうかを返します。
	/// </summary>
	bool HasEditorBillboard() const override { return true; }
	/// <summary>
	/// EditorBillboardIconNameを取得します。
	/// </summary>
	const char* GetEditorBillboardIconName() const override { return "icon_light_point.png"; }
	/// <summary>
	/// EditorBillboardPickRadiusを取得します。
	/// </summary>
	float GetEditorBillboardPickRadius() const override { return 0.65f; }

	/// <summary>
	/// Inspector表示
	/// </summary>
	void DrawInspector() override;

	/// <summary>
	/// PointLightのGPUデータへ追加する
	/// </summary>
	void Apply();

	/// <summary>
	/// Component情報をJSON形式で書き出す
	/// </summary>
	void WriteJson(nlohmann::json& json) const override;

	/// <summary>
	/// Component情報をJSON形式で読み込む
	/// </summary>
	void ReadJson(const nlohmann::json& json) override;

	/// <summary>
	/// ライトデータを取得
	/// </summary>
	PointLightData& GetData() { return data_; }

private:
	PointLightData data_{};
};

} // namespace KujakuEngine
