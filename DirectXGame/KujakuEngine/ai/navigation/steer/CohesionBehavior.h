#pragma once
#include "ISteeringBehavior.h"
#include <span>

namespace KujakuEngine {
namespace SteeringBehaviors {

/// <summary>
/// 周囲個体の中心へ寄る
/// </summary>
class CohesionBehavior : public ISteeringBehavior {
public:
	struct CohesionContext {
		std::span<const Vector3> neighbors;
	};

public:
	void SetContext(std::span<const Vector3> neighbors) {
		cohesionContext_.neighbors = neighbors;
	}

	Vector3 Calculate(const SteeringContext& context) const override;

private:
	CohesionContext cohesionContext_;
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine
