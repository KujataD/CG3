#include "AllyAIBrain.h"

#include "CharacterMotor.h"
#include "EnemyHealth.h"
#include "IAbilitySet.h"
#include "Player.h"
#include "PlayerHealth.h"

namespace {

constexpr const char* kBTSetFolder = "Resources/bt_set/AllyBT";

// 自分から見た相手への水平ベクトル。
KujakuEngine::Vector3 HorizontalTo(const KujakuEngine::GameObject& from, const KujakuEngine::GameObject& to) {
	KujakuEngine::Vector3 diff = to.GetTransform().translation_ - from.GetTransform().translation_;
	diff.y = 0.0f;
	return diff;
}

} // namespace

using namespace KujakuEngine;

void AllyAIBrain::Initialize() {

	// BTアクション登録
	// ---------------------------------------------
	BahamutAI::FunctionCatalog catalog;

	// 調整値はすべてBTノードのParamsで指定する(HammerEnemyと同方針。コンポーネント側には持たない)。

	BahamutAI::RegisterCondition(
	    btFactory_, catalog,
	    BahamutAI::ConditionDef("IsLeaderFar").Category("Sense").Description("リーダーがdistanceより遠いか").Float("distance", {10.0f}, "距離"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return IsLeaderFar(params);
	    });

	BahamutAI::RegisterCondition(
	    btFactory_, catalog,
	    BahamutAI::ConditionDef("IsEnemyInRange").Category("Sense").Description("range以内に敵がいるか").Float("range", {12.0f}, "索敵距離"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return IsEnemyInRange(params);
	    });

	BahamutAI::RegisterCondition(
	    btFactory_, catalog,
	    BahamutAI::ConditionDef("IsEnemyInAttackRange").Category("Sense").Description("敵が攻撃射程内か").Float("range", {2.5f}, "攻撃射程"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return IsEnemyInAttackRange(params);
	    });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("FollowLeader")
	        .Category("Movement")
	        .Description("リーダーへ歩く。stopDistance以内で停止")
	        .Float("stopDistance", {3.0f}, "停止距離")
	        .Float("speedScale", {1.0f}, "速度倍率"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return FollowLeader(params);
	    });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("StepToEnemy").Category("Movement").Description("最寄りの敵へ歩く").Float("speedScale", {1.0f}, "速度倍率"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return StepToEnemy(params);
	    });

	BahamutAI::RegisterAction(
	    btFactory_, catalog, BahamutAI::ActionDef("FaceEnemy").Category("Movement").Description("最寄りの敵へ旋回"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return FaceEnemy(params);
	    });

	BahamutAI::RegisterAction(
	    btFactory_, catalog, BahamutAI::ActionDef("UseAbility").Category("Attack").Description("技を使う").Int("slot", {0}, "技スロット"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return UseAbility(params);
	    });

	catalog.SaveToBTSetFolder(kBTSetFolder);
}

void AllyAIBrain::OnPlayStart() {
	motor_ = GetComponent<CharacterMotor>();
	abilitySet_ = GetComponent<IAbilitySet>();

	LoadBTSet();
}

void AllyAIBrain::Update() {
	if (!owner_ || !btRuntime_.IsLoaded()) {
		return;
	}

	// 監視オブザーバーは「実際にTickする個体」だけが初回Tickで遅延生成する。
	// 同一キー("Ally"=ツリー名)の最初の登録者だけがprimaryとして送信権を持つため、
	// OnPlayStartで作るとPartyManagerに無効化された側(Tickしない側)がprimaryを
	// 取ってしまい、BahamutAIEditorに実行フローが届かなくなる。
	if (!btObserver_) {
		btObserver_ = std::make_unique<BahamutAI::UdpTreeObserver>("Ally");
	}

	BahamutAI::AIContext context{localBlackboard_};
	context.deltaTime = Time::GetDeltaTime();
	context.SetOwner(*owner_);
	context.observer = btObserver_.get();

	btRuntime_.Tick(context);
}

void AllyAIBrain::RegisterInvokableMethods(KujakuEngine::InvokableMethodRegistry& registry) {
	registry.Add("LoadBTSet", [this]() { LoadBTSet(); });
}

void AllyAIBrain::LoadBTSet() {
	if (!btRuntime_.LoadFromBTSetFolder(kBTSetFolder, btFactory_)) {
		const BahamutAI::BehaviorTreeLoadResult& result = btRuntime_.GetLastLoadResult();
		KujakuEngine::Logger::Log(std::string("[AllyAIBrain] BT load failed: ") + result.GetErrorMessage());
	}
}

// ---------------------------------------------------------------------------
// BT Conditions
// ---------------------------------------------------------------------------

bool AllyAIBrain::IsLeaderFar(const BahamutAI::NodeParams& params) {
	GameObject* leader = FindLeader();
	if (!owner_ || !leader) {
		return false;
	}
	float distance = params.GetFloat("distance", 10.0f);
	return Length(HorizontalTo(*owner_, *leader)) > distance;
}

bool AllyAIBrain::IsEnemyInRange(const BahamutAI::NodeParams& params) {
	GameObject* enemy = FindNearestEnemy();
	if (!owner_ || !enemy) {
		return false;
	}
	float range = params.GetFloat("range", 12.0f);
	return Length(HorizontalTo(*owner_, *enemy)) <= range;
}

bool AllyAIBrain::IsEnemyInAttackRange(const BahamutAI::NodeParams& params) {
	GameObject* enemy = FindNearestEnemy();
	if (!owner_ || !enemy) {
		return false;
	}
	float range = params.GetFloat("range", 2.5f);
	return Length(HorizontalTo(*owner_, *enemy)) <= range;
}

// ---------------------------------------------------------------------------
// BT Actions
// ---------------------------------------------------------------------------

BahamutAI::BTStatus AllyAIBrain::FollowLeader(const BahamutAI::NodeParams& params) {
	GameObject* leader = FindLeader();
	if (!owner_ || !leader || !motor_) {
		return BahamutAI::BTStatus::Failure;
	}

	float stopDistance = params.GetFloat("stopDistance", 3.0f);
	float speedScale = params.GetFloat("speedScale", 1.0f);

	Vector3 toLeader = HorizontalTo(*owner_, *leader);
	if (Length(toLeader) <= stopDistance) {
		return BahamutAI::BTStatus::Success;
	}

	motor_->MoveWorld(toLeader, speedScale);
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus AllyAIBrain::StepToEnemy(const BahamutAI::NodeParams& params) {
	GameObject* enemy = FindNearestEnemy();
	if (!owner_ || !enemy || !motor_) {
		return BahamutAI::BTStatus::Failure;
	}

	float speedScale = params.GetFloat("speedScale", 1.0f);
	motor_->MoveWorld(HorizontalTo(*owner_, *enemy), speedScale);
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus AllyAIBrain::FaceEnemy(const BahamutAI::NodeParams& params) {
	(void)params;
	GameObject* enemy = FindNearestEnemy();
	if (!owner_ || !enemy || !motor_) {
		return BahamutAI::BTStatus::Failure;
	}

	motor_->FaceWorld(HorizontalTo(*owner_, *enemy));
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus AllyAIBrain::UseAbility(const BahamutAI::NodeParams& params) {
	if (!abilitySet_) {
		return BahamutAI::BTStatus::Failure;
	}

	int slot = params.GetInt("slot", 0);
	// クールダウン等で出せなくてもSuccess扱いにする(その場で構え続け、次のTickで再試行する)。
	abilitySet_->TryUse(slot);
	return BahamutAI::BTStatus::Success;
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

KujakuEngine::GameObject* AllyAIBrain::FindLeader() {
	if (!owner_ || !owner_->GetScene()) {
		return nullptr;
	}

	// 「自分以外のAllyタグで、入力頭脳(Player)が有効な者」=現在操作中のキャラ。
	// PartyManagerが頭脳を切り替えた結果を見るだけなので、直接の依存を持たない。
	GameObject* fallback = nullptr;
	for (const auto& object : owner_->GetScene()->GetGameObjects()) {
		if (!object || object.get() == owner_ || !object->IsActiveInHierarchy() || object->GetTag() != "Ally") {
			continue;
		}
		if (!object->GetComponent<CharacterMotor>()) {
			continue;
		}
		Player* inputBrain = object->GetComponent<Player>();
		if (inputBrain && inputBrain->IsEnabled()) {
			return object.get();
		}
		// 入力頭脳が見つからない場合の保険(PartyManager未設定でもそれらしく動く)。
		if (!fallback) {
			fallback = object.get();
		}
	}
	return fallback;
}

KujakuEngine::GameObject* AllyAIBrain::FindNearestEnemy() {
	if (!owner_ || !owner_->GetScene()) {
		return nullptr;
	}

	GameObject* nearest = nullptr;
	float nearestDistSq = 0.0f;
	const Vector3 selfPosition = owner_->GetTransform().translation_;
	for (const auto& object : owner_->GetScene()->GetGameObjects()) {
		if (!object || !object->IsActiveInHierarchy()) {
			continue;
		}
		EnemyHealth* health = object->GetComponent<EnemyHealth>();
		if (!health || !health->IsAlive()) {
			continue;
		}
		Vector3 diff = object->GetTransform().translation_ - selfPosition;
		diff.y = 0.0f;
		float distSq = diff.x * diff.x + diff.z * diff.z;
		if (!nearest || distSq < nearestDistSq) {
			nearest = object.get();
			nearestDistSq = distSq;
		}
	}
	return nearest;
}
