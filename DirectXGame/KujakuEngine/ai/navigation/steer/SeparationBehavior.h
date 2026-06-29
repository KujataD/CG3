#pragma once
#include "ISteeringBehavior.h"
#include <span>
#include <vector>

namespace KujakuEngine {

namespace SteeringBehaviors {
/// <summary>
/// 集団を分散させます。
/// 群れなどの過密を防ぐことができます。
/// </summary>
class SeparationBehavior : public ISteeringBehavior {
public:
	struct SeparationContext {
		std::span<const Vector3> neighbors;
		float separationRange = 1.0f;
	};

public:
	/// <summary>
	/// 計算
	/// </summary>
	/// <param name="context"></param>
	/// <returns></returns>
	Vector3 Calculate(const SteeringContext& context) const override;

	/// <summary>
	/// メンバーを追加
	/// </summary>
	/// <param name="member"></param>
	void SetContext(const std::span<const Vector3> neighbors, float range) {
		separationContext_.neighbors = neighbors;
		separationContext_.separationRange = range;
	}

private:
	SeparationContext separationContext_;
};
} // namespace SteeringBehaviors
} // namespace KujakuEngine