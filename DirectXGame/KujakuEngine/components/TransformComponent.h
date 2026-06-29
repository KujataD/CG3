#pragma once

#include "../3d/WorldTransform.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// GameObjectが必ず持つTransform Component
/// </summary>
class TransformComponent : public Component {
public:
	const char* GetTypeName() const override { return "Transform"; }
	void Initialize() override;
	void DrawInspector() override;
	void WriteJson(std::ostream& os, int indent) const override;
	bool CanRemove() const override { return false; }
	bool AllowMultiple() const override { return false; }

	/// <summary>
	/// WorldTransformを取得
	/// </summary>
	WorldTransform& GetTransform() { return transform_; }

	/// <summary>
	/// WorldTransformを取得
	/// </summary>
	const WorldTransform& GetTransform() const { return transform_; }

private:
	WorldTransform transform_;
	bool initialized_ = false;
};

} // namespace KujakuEngine
