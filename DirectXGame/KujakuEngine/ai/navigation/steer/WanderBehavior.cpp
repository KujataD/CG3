#include "WanderBehavior.h"
#include <algorithm>
#include <math/Random.h>

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 WanderBehavior::Calculate(const SteeringContext& context) const {
	// 進行方向を求める
	Vector3 forward = context.velocity;

	// 進行方向がない場合は、前方を向く
	if (Length(forward) == 0.0f) {
		forward = {0.0f, 0.0f, 1.0f};
	} 
	// 進行方向がある場合は、正規化する
	else {
		forward = Normalize(forward);
	}

	// ワンダー角をランダムに変化させる
	wanderAngle_ += Random::GetRandom(-param_.angleChange, param_.angleChange);

	// ワンダーサークルの中心と、ワンダー角からの変位を求める
	Vector3 circleCenter = forward * param_.circleDistance;

	// ワンダーサークル上の点への変位
	Vector3 displacement = {
		std::sin(wanderAngle_) * param_.circleRadius,
		0.0f,
		std::cos(wanderAngle_) * param_.circleRadius,
	};

	// 目標速度を求め、ステアリングを求める
	Vector3 desired = Normalize(circleCenter + displacement) * context.maxSpeed;
	Vector3 steering = desired - context.velocity;
	return Limit(steering, context.maxForce);
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine
