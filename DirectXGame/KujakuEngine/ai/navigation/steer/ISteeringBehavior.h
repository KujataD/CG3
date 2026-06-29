#pragma once
#include <math/MathUtil.h>
#include <memory>
namespace KujakuEngine {

namespace SteeringBehaviors {

/// <summary>
/// ステアリングに必要なパラメータです。
/// </summary>
struct SteeringContext {
	Vector3 position;
	Vector3 velocity;

	float maxSpeed = 1.0f;
	float maxForce = 0.1f;
};

/// <summary>
/// AIの移動を自然にするクラスの基底クラスです。
/// </summary>
class ISteeringBehavior {
public:
	virtual ~ISteeringBehavior() = default;

	virtual Vector3 Calculate(const SteeringContext& context) const = 0;

	Vector3 operator()(const SteeringContext& context) const { return Calculate(context); }
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine