#pragma once
#include "ISteeringBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

/// <summary>
/// ランダムにうろつく
/// </summary>
class WanderBehavior : public ISteeringBehavior {
public:
	/// <summary>
	/// WanderContext構造体を表します。
	/// </summary>
	struct WanderContext {
		float circleDistance = 2.0f;
		float circleRadius = 1.0f;
		float angleChange = 0.5f;
	};

public:
	void SetParam(const WanderContext& param) { param_ = param; }
	void SetWanderAngle(float angle) { wanderAngle_ = angle; }

	/// <summary>
	/// ulateを計算します。
	/// </summary>
	Vector3 Calculate(const SteeringContext& context) const override;

private:
	WanderContext param_;
	mutable float wanderAngle_ = 0.0f;
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine
