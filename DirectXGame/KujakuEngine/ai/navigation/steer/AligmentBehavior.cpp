#include "AligmentBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 AligmentBehavior::Calculate(const SteeringContext& context) const {
	// 隣接個体の速度がない場合は、ステアリングもなし
	if (aligmentContext_.neighborVelocities.empty()) {
		return {};
	}

	// 近接個体の平均速度を求める
	Vector3 averageVelocity{};
	for (const Vector3& velocity : aligmentContext_.neighborVelocities) {
		averageVelocity += velocity;
	}
	averageVelocity /= static_cast<float>(aligmentContext_.neighborVelocities.size());

	// 平均速度が0の場合は、ステアリングもなし
	if (Length(averageVelocity) == 0.0f) {
		return {};
	}

	// 平均速度の方向へ向くステアリングを求める
	Vector3 desired = Normalize(averageVelocity) * context.maxSpeed;

	// 現在の速度から、desiredな速度へ向かうためのステアリングを求める
	Vector3 steering = desired - context.velocity;

	// ステアリングの大きさを制限して返す
	return Limit(steering, context.maxForce);
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine
