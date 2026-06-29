#include "SeekBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 SeekBehavior::Calculate(const SteeringContext& context) const {
	// 目標速度を求める
	Vector3 desired = Normalize(target_ - context.position) * context.maxSpeed;

	// どれくらい速度を変化させるか
	Vector3 steering = desired - context.velocity;

	// 急なハンドリング防止
	return Limit(steering, context.maxForce);
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine