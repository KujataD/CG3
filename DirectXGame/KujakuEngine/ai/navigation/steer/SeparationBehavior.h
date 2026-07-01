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
	/// <summary>
	/// SeparationContext構造体を表します。
	/// </summary>
	struct SeparationContext {
		std::span<const Vector3> neighbors;
		float separationRange = 1.0f;
	};

public:
	/// <param name="context"></param>
	/// <returns></returns>
	/// <summary>
	/// ulateを計算します。
	/// </summary>
	Vector3 Calculate(const SteeringContext& context) const override;

	/// <param name="member"></param>
	/// <summary>
	/// Contextを設定します。
	/// </summary>
	void SetContext(const std::span<const Vector3> neighbors, float range) {
		separationContext_.neighbors = neighbors;
		separationContext_.separationRange = range;
	}

private:
	SeparationContext separationContext_;
};
} // namespace SteeringBehaviors
} // namespace KujakuEngine