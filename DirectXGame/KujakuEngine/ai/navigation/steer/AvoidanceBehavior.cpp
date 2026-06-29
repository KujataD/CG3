#include "AvoidanceBehavior.h"
#include <math/MathUtil.h>

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 AvoidanceBehavior::Calculate(const SteeringContext& context) const {
	if (context_.obstacles.empty()) {
		return {};
	}

	// 回避速度
	Vector3 avoid{};

	// 進行方向
	Vector3 forward;

	forward.x = cosf(context_.rotation.x) * sinf(context_.rotation.y);
	forward.y = sinf(context_.rotation.x);
	forward.z = cosf(context_.rotation.x) * cosf(context_.rotation.y);

	forward = Normalize(forward);

	Segment rays[3];

	// 正面のRay
	rays[0].origin = context.position;
	rays[0].diff = forward * context_.rayLength;

	rays[1].origin = context.position;
	rays[1].diff.x = forward.x * std::cos(context_.fov) - forward.z * std::sin(context_.fov);
	rays[1].diff.y = forward.y;
	rays[1].diff.z = forward.x * std::sin(context_.fov) + forward.z * std::cos(context_.fov);
	rays[1].diff = Normalize(rays[1].diff) * context_.rayLength;

	rays[2].origin = context.position;
	rays[2].diff.x = forward.x * std::cos(-context_.fov) - forward.z * std::sin(-context_.fov);
	rays[2].diff.y = forward.y;
	rays[2].diff.z = forward.x * std::sin(-context_.fov) + forward.z * std::cos(-context_.fov);
	rays[2].diff = Normalize(rays[2].diff) * context_.rayLength;

	for (const AABB& obstacle : context_.obstacles) {
		for (int i = 0; i < 3; i++) {
			if (ShapeUtil::IsCollision(obstacle, rays[i])) {
				// 壁の中心から狼へ向かう方向
				Vector3 wallCenter = (obstacle.min + obstacle.max) * 0.5f;
				Vector3 awayDirection = Normalize(context.position - wallCenter);

				avoid += awayDirection;
			}
		}
	}
	return Normalize(avoid) * context.maxForce;
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine