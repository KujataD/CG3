#pragma once
#include <KujakuEngine.h>
#include <components/AnimatorComponent.h>

class Player : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "Player"; }

	void Initialize() override;
	void OnPlayStart() override;
	void Update() override;

	/// <summary>
	/// ノックバックを与える。stunDurationの間は入力移動できず、初速velocityから減衰しながら滑る。
	/// </summary>
	void ApplyKnockback(const KujakuEngine::Vector3& velocity, float stunDuration);

	/// <summary>硬直中(被弾でしばらく動けない状態)か。</summary>
	bool IsStunned() const { return stunTimer_ > 0.0f; }

private:

	void Move();
	void UpdateKnockback(float deltaTime);
	/// <summary>
	/// モデル(子)に溜まったルートモーションの水平移動分を最上位(コライダー側)へ移す。
	/// 攻撃の踏み込み(moveForward)で実際の当たり判定も前進させるため。
	/// 上下(y)はPlayerWalkのボビング等の見た目専用なので移さない。
	/// </summary>
	void TransferModelRootMotion();

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.05f, 0.0f, 100.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(turnSpeed_, "Turn Speed", 0.1f, 0.0f, 50.0f);
		KUJAKU_REGISTER_FLOAT_NAMED(knockbackDamping_, "Knockback Damping", 0.1f, 0.0f, 50.0f);
	}

	// 移動速度[unit/s]
	KUJAKU_FIELD_FLOAT(speed_, 5.0f);
	// 旋回速度[rad/s]
	KUJAKU_FIELD_FLOAT(turnSpeed_, 12.0f);
	// ノックバック速度の減衰の強さ(大きいほどすぐ止まる)
	KUJAKU_FIELD_FLOAT(knockbackDamping_, 6.0f);

	// モデル側(子のPawnModel)のAnimator。コリジョンはこの最上位、見た目とアニメーションは子が担当する。
	KujakuEngine::AnimatorComponent* animator_ = nullptr;
	// Animatorを持つモデルのGameObject(ルートモーション転送先の参照元)。
	KujakuEngine::GameObject* modelObject_ = nullptr;

	// --- 被弾状態 ---
	// 残り硬直時間[s]。0より大きい間は入力移動を受け付けない。
	float stunTimer_ = 0.0f;
	// ノックバックの現在速度。指数減衰させながらtranslationへ加算する。
	KujakuEngine::Vector3 knockbackVelocity_ = {0.0f, 0.0f, 0.0f};
};
