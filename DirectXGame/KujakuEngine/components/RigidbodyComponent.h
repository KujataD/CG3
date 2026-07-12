#pragma once

#include "../math/Vector3.h"
#include "../scene/Component.h"

namespace KujakuEngine {

/// <summary>
/// 剛体反発(押し出し+速度反射)のためのRigidbody。形状を持つColliderとは分離する。
/// 重力・摩擦・力は扱わない。速度はゲームコードがSetVelocityで与えるか、反射で変化する(自然落下しない)。
///
/// Is Static:
///   OFF(動的) = 通常の剛体。衝突で押し出され、速度が反射される。
///   ON(キネマティック) = 無限質量として扱う。衝突で押し出されず速度も変化しないが、
///     自身のvelocityで移動し(動く床など)、接触した動的ボディへ運動量を与える。
/// Rigidbodyを持たないCollider = 動かない純粋な静的壁(velocity 0、運動量なし)。
/// </summary>
class RigidbodyComponent : public Component {
public:
	const char* GetTypeName() const override { return "RigidbodyComponent"; }

	// 同一GameObjectに複数のRigidbodyは持てない。
	bool AllowMultiple() const override { return false; }

	const Vector3& GetVelocity() const { return velocity_; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

	// 反発係数[0,1]。0=完全非弾性、1=完全弾性。
	float GetBounciness() const { return bounciness_; }
	void SetBounciness(float bounciness) {
		if (bounciness < 0.0f) {
			bounciness = 0.0f;
		}
		if (bounciness > 1.0f) {
			bounciness = 1.0f;
		}
		bounciness_ = bounciness;
	}

	// true=キネマティック(無限質量・押し出されない)、false=動的。
	bool IsStatic() const { return isStatic_; }
	void SetStatic(bool isStatic) { isStatic_ = isStatic; }

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_BOOL(isStatic_);
		KUJAKU_REGISTER_VECTOR3(velocity_, 0.01f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(bounciness_, 0.01f, 0.0f, 1.0f);
	}

	KUJAKU_FIELD_BOOL(isStatic_, false);
	KUJAKU_FIELD_VECTOR3(velocity_, {});
	KUJAKU_FIELD_FLOAT(bounciness_, 0.5f);
};

} // namespace KujakuEngine
