#include "CohesionBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 CohesionBehavior::Calculate(const SteeringContext& context) const {
	if (cohesionContext_.neighbors.empty()) {
		return {};
	}

	// 周囲の個体の中心を求める
	Vector3 center{};
	for (const Vector3& neighbor : cohesionContext_.neighbors) {
		center += neighbor;
	}
	center /= static_cast<float>(cohesionContext_.neighbors.size());

	// 中心へ向かうベクトルを求める
	Vector3 toCenter = center - context.position;

	// 中心にいる場合は、ステアリングなし
	if (Length(toCenter) == 0.0f) {
		return {};
	}

	// 目標速度を求め、ステアリングを求める
	Vector3 desired = Normalize(toCenter) * context.maxSpeed;
	Vector3 steering = desired - context.velocity;
	return Limit(steering, context.maxForce);
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine
