#pragma once

#include "../3d/WorldTransform.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// GameObjectが必ず持つTransform Component
/// </summary>
class TransformComponent : public Component {
public:
	/// <summary>
	/// TypeNameを取得します。
	/// </summary>
	const char* GetTypeName() const override { return "Transform"; }
	/// <summary>
	/// 初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// Inspectorを描画します。
	/// </summary>
	void DrawInspector() override;
	/// <summary>
	/// WriteJsonを実行します。
	/// </summary>
	void WriteJson(nlohmann::json& json) const override;
	/// <summary>
	/// ReadJsonを実行します。
	/// </summary>
	void ReadJson(const nlohmann::json& json) override;
	/// <summary>
	/// Removeできるかどうかを返します。
	/// </summary>
	bool CanRemove() const override { return false; }
	/// <summary>
	/// AllowMultipleを実行します。
	/// </summary>
	bool AllowMultiple() const override { return false; }
	/// <summary>
	/// TransformComponentかどうかを返します。
	/// </summary>
	bool IsTransformComponent() const override { return true; }

	/// <summary>
	/// WorldTransformを取得
	/// </summary>
	WorldTransform& GetTransform();

	/// <summary>
	/// WorldTransformを取得
	/// </summary>
	const WorldTransform& GetTransform() const;

private:
	WorldTransform fallbackTransform_;
	bool initialized_ = false;
};

} // namespace KujakuEngine
