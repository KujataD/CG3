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
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;
	bool CanRemove() const override { return false; }
	bool AllowMultiple() const override { return false; }
	bool IsTransformComponent() const override { return true; }

	WorldTransform& GetTransform();

	const WorldTransform& GetTransform() const;

private:
	WorldTransform fallbackTransform_;
	bool initialized_ = false;
};

} // namespace KujakuEngine
