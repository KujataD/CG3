#pragma once
#include "ISteeringBehavior.h"

namespace KujakuEngine {
namespace SteeringBehaviors {

/// <summary>
/// 目標までの速度を求めます
/// 主に追尾ロケット等の移動に役立ちます。
/// </summary>
class SeekBehavior : public ISteeringBehavior {
public:
	void SetTarget(Vector3 target) { target_ = target; }

	/// <summary>
	/// ulateを計算します。
	/// </summary>
	Vector3 Calculate(const SteeringContext& context) const override;

private:
	Vector3 target_{};
};

} // namespace SteeringBehaviors
} // namespace KujakuEngine