#pragma once

#include <BahamutAI/AI.h>
#include <KujakuEngine.h>
#include <memory>
#include <string>

namespace KujakuEngine {
class AnimatorComponent;
}

/// <summary>
/// ハンマーを持つ敵。BehaviorTree(BahamutAI)で行動する。
/// ツリー本体はBahamutAIEditorで作成し、ここではActionの実装と登録だけを行う。
///
/// 登録アクション:
///   MoveToPlayer   : プレイヤーへ歩いて近づく(speed / stopDistance)
///   FacePlayer     : プレイヤーの方向へ旋回する(turnSpeed)
///   IsPlayerInRange: プレイヤーが範囲内ならSuccess(range)
///   SpinAttack     : パターン1 回転ハンマー(振りかぶり→2秒回転切り)
///   SlamAttack     : パターン2 上段たたきつけ(振りかぶり→溜め→歩きながらたたきつけ)
///
/// ハンマー(子孫オブジェクト"HammerPivot")は攻撃中だけSetActiveで取り出す。
/// 攻撃アニメーションはHammerPivotの回転をキーフレームし、EnemyWeapon.attackで判定をON/OFFする。
///
/// 被弾時はのけぞり(HammerRecoil)を再生する。のけぞるかどうかは状態ごとのパラメータで制御し、
/// 既定では回転攻撃(SpinAttack)中はのけぞらない。のけぞり中は行動アクションがFailureを返す。
/// </summary>
class HammerEnemyComponent : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "HammerEnemyComponent"; }

	void Initialize() override;
	void OnPlayStart() override;
	void Update() override;

	void RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) override;

private:
	void LoadBTSet();

	// --- BT Actions ---
	BahamutAI::BTStatus MoveToPlayer(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus FacePlayer(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus IsPlayerInRange(const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus SpinAttack(BahamutAI::AIContext& context);
	BahamutAI::BTStatus SlamAttack(BahamutAI::AIContext& context);

	// --- helpers ---
	KujakuEngine::GameObject* FindPlayer();
	KujakuEngine::GameObject* FindHammerPivot();
	KujakuEngine::AnimatorComponent* GetAnimator();
	/// 被弾時に呼ばれる。状態ごとのパラメータを見て、のけぞりを再生するか決める。
	void OnDamaged();
	/// のけぞり(硬直)中か。
	bool IsRecoiling() const { return recoilTimer_ > 0.0f; }
	void SetHammerVisible(bool visible);
	/// プレイヤーへYawだけ旋回する。deltaTimeぶん回して、ほぼ向いていればtrue。
	bool RotateTowardsPlayer(float turnSpeed, float deltaTime);
	/// 現在の向きへ関係なく、プレイヤーへ直線的にdeltaTimeぶん歩く。
	void MoveTowardsPlayer(float speed, float deltaTime);
	/// 攻撃終了/中断時の後始末(ハンマー収納・フラグリセット)。
	void FinishAttack();

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_STRING(playerName_);
		KUJAKU_REGISTER_FLOAT(walkSpeed_, 0.05f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(turnSpeed_, 0.05f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(slamWalkStart_, 0.01f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(slamWalkEnd_, 0.01f, 0.0f, 0.0f);
		KUJAKU_REGISTER_FLOAT(slamWalkSpeed_, 0.05f, 0.0f, 0.0f);
		KUJAKU_REGISTER_BOOL_NAMED(recoilEnabled_, "Recoil Enabled");
		KUJAKU_REGISTER_BOOL_NAMED(recoilDuringSpin_, "Recoil During Spin");
		KUJAKU_REGISTER_BOOL_NAMED(recoilDuringSlam_, "Recoil During Slam");
		KUJAKU_REGISTER_FLOAT_NAMED(recoilDuration_, "Recoil Duration", 0.05f, 0.0f, 5.0f);
	}

	// プレイヤー(PlayerHealth持ち)のGameObject名。シーンから検索する。
	KUJAKU_FIELD_STRING(playerName_, "Pawn");
	KUJAKU_FIELD_FLOAT(walkSpeed_, 2.0f);
	KUJAKU_FIELD_FLOAT(turnSpeed_, 6.0f);
	// SlamAttack中にプレイヤーへ歩き出す/歩き終わる時刻(クリップ先頭からの秒)。
	// HammerSlam.anim.jsonの「溜め終わり→たたきつけ」区間に合わせる。
	KUJAKU_FIELD_FLOAT(slamWalkStart_, 1.4f);
	KUJAKU_FIELD_FLOAT(slamWalkEnd_, 1.9f);
	KUJAKU_FIELD_FLOAT(slamWalkSpeed_, 4.0f);

	// --- のけぞり制御 ---
	// のけぞり自体の有効/無効。
	KUJAKU_FIELD_BOOL(recoilEnabled_, true);
	// 回転攻撃(SpinAttack)中に被弾したときのけぞるか。既定はのけぞらない(スーパーアーマー)。
	KUJAKU_FIELD_BOOL(recoilDuringSpin_, false);
	// たたきつけ(SlamAttack)中に被弾したときのけぞるか。のけぞると攻撃は中断される。
	KUJAKU_FIELD_BOOL(recoilDuringSlam_, true);
	// のけぞりの硬直時間[s](HammerRecoil.anim.jsonの長さに合わせる)。
	KUJAKU_FIELD_FLOAT(recoilDuration_, 0.5f);

	// BT作成用
	BahamutAI::BehaviorTreeFactory btFactory_;

	// BT実行本体(毎フレームTickする)
	BahamutAI::BehaviorTreeRuntime btRuntime_;

	// ライブ監視オブザーバー。Playインスタンスだけが遅延生成する(EnemyComponentと同じ方針)。
	std::unique_ptr<BahamutAI::UdpTreeObserver> btObserver_;

	// このEnemy固有のBlackboard(AIContextに渡す)
	BahamutAI::Blackboard localBlackboard_;

	// --- 攻撃アクションの実行状態 ---
	// SpinAttack/SlamAttackはRunningをまたぐため、開始済みかを覚えておく。
	bool spinActive_ = false;
	bool slamActive_ = false;
	float slamTimer_ = 0.0f;

	// のけぞりの残り硬直時間[s]。0より大きい間は行動アクションがFailureを返す。
	float recoilTimer_ = 0.0f;
};
