#pragma once
#include "ISteeringBehavior.h"
#include <span>

namespace KujakuEngine {
namespace SteeringBehaviors {

/// <summary>
/// 整列してうごく
/// </summary>
class AligmentBehavior : public ISteeringBehavior {
public:
	struct AligmentContext {
		std::span<const Vector3> neighborVelocities;
	};

public:
	void SetContext(std::span<const Vector3> neighborVelocities) {
		aligmentContext_.neighborVelocities = neighborVelocities;
	}

	Vector3 Calculate(const SteeringContext& context) const override;

private:
	AligmentContext aligmentContext_;
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine
