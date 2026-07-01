#pragma once
#include "ISteeringBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

/// <summary>
/// 到着地点に近づくほど、ゆっくりになる
/// </summary>
class ArriveBehavior : public ISteeringBehavior {
public:
	void SetTarget(Vector3 target) { target_ = target; }
	void SetSlowRadius(float slowRadius) { slowRadius_ = slowRadius; }

	/// <summary>
	/// ulateを計算します。
	/// </summary>
	Vector3 Calculate(const SteeringContext& context) const override;

private:
	Vector3 target_{};
	float slowRadius_ = 3.0f;
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine
