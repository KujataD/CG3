#include "ArriveBehavior.h"
#include <algorithm>

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 ArriveBehavior::Calculate(const SteeringContext& context) const {
	// ターゲットへのベクトル
	Vector3 toTarget = target_ - context.position;

	// ターゲット距離
	float distance = Length(toTarget);

	// ターゲットに到着している場合は、減速せずに停止する。
	if (distance == 0.0f) {
		return Limit(-context.velocity, context.maxForce);
	}

	// ターゲットに近づくにつれて減速するように、ターゲット距離に応じて目標速度を変化させる。
	float radius = std::max(slowRadius_, 0.001f);
	float speedRate = std::clamp(distance / radius, 0.0f, 1.0f);

	// 目標速度までのステアリングを計算
	Vector3 desired = Normalize(toTarget) * context.maxSpeed * speedRate;
	Vector3 steering = desired - context.velocity;
	return Limit(steering, context.maxForce);
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine
