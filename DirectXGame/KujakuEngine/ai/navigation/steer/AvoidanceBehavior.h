#pragma once
#include "ISteeringBehavior.h"
#include <shapes/ShapeUtil.h>
#include <span>
#include <numbers>

namespace KujakuEngine {
namespace SteeringBehaviors {

/// <summary>
/// 障害物をよけるように動きます。
/// </summary>
class AvoidanceBehavior : public ISteeringBehavior {
public:
	
	struct AvoidanceContext {
		Vector3 rotation;

		// 障害物
		std::span<const AABB> obstacles;

		// 光線視野角（ラジアン）
		float fov = std::numbers::pi_v<float> * 0.25f;

		// 光線の大きさ
		float rayLength = 3.0f;
	};

public:

	void SetContext(const AvoidanceContext& param) { context_ = param; }

	Vector3 Calculate(const SteeringContext& context) const override;

private:
	AvoidanceContext context_;
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine