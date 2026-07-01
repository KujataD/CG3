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
	/// <summary>
	/// ISteeringBehaviorを実行します。
	/// </summary>
	virtual ~ISteeringBehavior() = default;

	/// <summary>
	/// ulateを計算します。
	/// </summary>
	virtual Vector3 Calculate(const SteeringContext& context) const = 0;

	/// <summary>
	/// operatorを実行します。
	/// </summary>
	Vector3 operator()(const SteeringContext& context) const { return Calculate(context); }
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine