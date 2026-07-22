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

	/// <summary>回避(前転)中か。</summary>
	bool IsDodging() const { return dodgeTimer_ > 0.0f; }

	/// <summary>回避後のクールダウン中か(再回避のみ不可。移動や攻撃は可能)。</summary>
	bool IsDodgeCooldown() const { return dodgeCooldownTimer_ > 0.0f; }

	/// <summary>行動不能(硬直・回避中)か。攻撃入力などの抑制に使う。クールダウンは含まない。</summary>
	bool IsActionLocked() const { return IsStunned() || IsDodging(); }

private:

	void Move();
	void UpdateKnockback(float deltaTime);
	/// <summary>回避を開始する(無敵付与+前転アニメーション再生)。</summary>
	void StartDodge();
	/// <summary>回避中の前進と終了判定。終了時にクールダウンへ移行する。</summary>
	void UpdateDodge(float deltaTime);
	/// <summary>無敵時間の残りを進め、切れたら無敵を解除する。</summary>
	void TickInvincibility(float deltaTime);
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
		KUJAKU_REGISTER_INT_NAMED(dodgeDurationFrames_, "Dodge Duration Frames", 1.0f, 1, 600);
		KUJAKU_REGISTER_INT_NAMED(invincibleFrames_, "Dodge Invincible Frames", 1.0f, 0, 600);
		KUJAKU_REGISTER_INT_NAMED(dodgeCooldownFrames_, "Dodge Cooldown Frames", 1.0f, 0, 600);
		KUJAKU_REGISTER_FLOAT_NAMED(dodgeSpeed_, "Dodge Speed", 0.1f, 0.0f, 100.0f);
	}

	// 移動速度[unit/s]
	KUJAKU_FIELD_FLOAT(speed_, 5.0f);
	// 旋回速度[rad/s]
	KUJAKU_FIELD_FLOAT(turnSpeed_, 12.0f);
	// ノックバック速度の減衰の強さ(大きいほどすぐ止まる)
	KUJAKU_FIELD_FLOAT(knockbackDamping_, 6.0f);

	// --- 回避 ---
	// 回避(前転)の長さ[フレーム, 60fps基準]。PlayerRoll.anim.jsonの尺(0.5秒=30F)に合わせるとよい。
	KUJAKU_FIELD_INT(dodgeDurationFrames_, 30);
	// 回避開始からの無敵時間[フレーム]。
	KUJAKU_FIELD_INT(invincibleFrames_, 27);
	// 回避終了後、再び回避できるまでのクールダウン[フレーム](移動や攻撃は可能)。
	KUJAKU_FIELD_INT(dodgeCooldownFrames_, 10);
	// 回避中の前進速度[unit/s]。
	KUJAKU_FIELD_FLOAT(dodgeSpeed_, 8.0f);

	// モデル側(子のPawnModel)のAnimator。コリジョンはこの最上位、見た目とアニメーションは子が担当する。
	KujakuEngine::AnimatorComponent* animator_ = nullptr;
	// Animatorを持つモデルのGameObject(ルートモーション転送先の参照元)。
	KujakuEngine::GameObject* modelObject_ = nullptr;

	// --- 被弾状態 ---
	// 残り硬直時間[s]。0より大きい間は入力移動を受け付けない。
	float stunTimer_ = 0.0f;
	// ノックバックの現在速度。指数減衰させながらtranslationへ加算する。
	KujakuEngine::Vector3 knockbackVelocity_ = {0.0f, 0.0f, 0.0f};

	// --- 回避状態 ---
	// 回避の残り時間[s]。0より大きい間は前転しながら前進し、他の行動はできない。
	float dodgeTimer_ = 0.0f;
	// 回避クールダウンの残り時間[s]。0より大きい間は再回避できない(移動や攻撃は可能)。
	float dodgeCooldownTimer_ = 0.0f;
	// 無敵の残り時間[s]。切れたらPlayerHealthの無敵を解除する。
	float invincibleTimer_ = 0.0f;

	// 同じGameObjectのPlayerHealth(無敵の付与先)。
	class PlayerHealth* health_ = nullptr;
};
