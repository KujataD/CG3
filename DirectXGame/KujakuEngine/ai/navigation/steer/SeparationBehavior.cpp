#include "SeparationBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

Vector3 SeparationBehavior::Calculate(const SteeringContext& context) const {
	Vector3 steering{};

	for (const auto& neighbor : separationContext_.neighbors) {
		// メンバー → 自身のベクトルを求める。
		Vector3 difference = context.position - neighbor;

		// 距離
		float distance = Length(difference);

		if (distance < separationContext_.separationRange) {
			steering += Normalize(difference) / distance;
		}
	}

	return Limit(steering, context.maxForce);
}

} // namespace SteeringBehaviors
} // namespace KujakuEngine