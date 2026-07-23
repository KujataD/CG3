#pragma once

#include <BahamutAI/AI.h>
#include <KujakuEngine.h>
#include <memory>

class CharacterMotor;
class IAbilitySet;

/// <summary>
/// 味方NPCの「頭脳」(BehaviorTree)。操作されていない側のプレイアブルキャラを
/// エルデンリングの霊灰のように参戦させる。PartyManagerが入力頭脳(Player)と排他で有効化する。
///
/// 体は同じGameObjectのCharacterMotor、攻撃はIAbilitySet(Melee/Magic)を使うため、
/// PawnにもBishopにも同じままで付けられる。射程などの性格付けはInspectorの数値で行う
/// (近接のPawnはAttack Rangeを小さく、魔法のBishopは大きく)。
///
/// ツリー本体はBahamutAIEditorでbt_set/AllyBTに作成する(HammerEnemyと同じ流儀)。
/// ここではCondition/Actionの実装と登録だけを行う。
/// 調整パラメータ(距離・倍率など)はすべてBTノードのParamsで指定する(コンポーネント側には持たない)。
///
/// 登録Condition: IsLeaderFar(distance) / IsEnemyInRange(range) / IsEnemyInAttackRange(range)
/// 登録Action:    FollowLeader(stopDistance, speedScale) / StepToEnemy(speedScale)
///                FaceEnemy() / UseAbility(slot)
///
/// 攻撃射程などのキャラ差(近接のPawnは短く、魔法のBishopは長く)を付けたい場合は、
/// ツリー側のParams(IsEnemyInAttackRangeのrange等)を調整するか、bt_setをキャラ別に分ける。
/// </summary>
class AllyAIBrain : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "AllyAIBrain"; }
	bool AllowMultiple() const override { return false; }

	void Initialize() override;
	void OnPlayStart() override;
	void Update() override;

	void RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) override;

private:
	void LoadBTSet();

	// --- BT Conditions ---
	bool IsLeaderFar(const BahamutAI::NodeParams& params);
	bool IsEnemyInRange(const BahamutAI::NodeParams& params);
	/// <summary>敵が攻撃射程内か(索敵のIsEnemyInRangeとはデフォルト射程が異なるだけ)。</summary>
	bool IsEnemyInAttackRange(const BahamutAI::NodeParams& params);

	// --- BT Actions(HammerEnemyと同じく毎Tick1歩だけ進めてSuccessを返し、ループはRootの再評価に任せる) ---
	BahamutAI::BTStatus FollowLeader(const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus StepToEnemy(const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus FaceEnemy(const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus UseAbility(const BahamutAI::NodeParams& params);

	// --- helpers ---
	/// <summary>リーダー(操作中キャラ)を探す。「自分以外のAllyタグでPlayer(入力頭脳)が有効な者」。</summary>
	KujakuEngine::GameObject* FindLeader();
	/// <summary>最寄りの生存敵(EnemyHealth持ち)を探す。</summary>
	KujakuEngine::GameObject* FindNearestEnemy();

private:
	// BT作成用
	BahamutAI::BehaviorTreeFactory btFactory_;
	// BT実行本体(毎フレームTickする)
	BahamutAI::BehaviorTreeRuntime btRuntime_;
	// ライブ監視オブザーバー(Playインスタンスだけが遅延生成)
	std::unique_ptr<BahamutAI::UdpTreeObserver> btObserver_;
	// このキャラ固有のBlackboard
	BahamutAI::Blackboard localBlackboard_;

	CharacterMotor* motor_ = nullptr;
	IAbilitySet* abilitySet_ = nullptr;
};
