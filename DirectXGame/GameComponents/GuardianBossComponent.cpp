#include "GuardianBossComponent.h"
#include "GameFx.h"
#include <components/ParticleSystemComponent.h>

#include "EnemyHealth.h"
#include "HateTable.h"
#include "EnemyWeapon.h"
#include "GuardianBody.h"
#include "GuardianGait.h"
#include "GuardianRigMath.h"
#include "IGuardianLegRig.h"
#include "PlayerHealth.h"

#include <Editor/PrefabAsset.h>
#include <components/ModelRendererComponent.h>
#include <components/DecalComponent.h>

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KujataEngine;

namespace {

std::string BTSetFolder() { return (KujataEngine::GetProjectDataRoot() / "Resources" / "bt_set" / "GuardianBT").generic_string(); }

constexpr float kDegreeToRadian = std::numbers::pi_v<float> / 180.0f;

// 角度を[-π, π]へ折り返す(KujataEngine::WrapAngleはDLLからリンクできないのでローカルに持つ)。
float WrapYawAngle(float angle) {
	constexpr float pi = std::numbers::pi_v<float>;
	angle = std::fmod(angle + pi, 2.0f * pi);
	if (angle < 0.0f) {
		angle += 2.0f * pi;
	}
	return angle - pi;
}

// 自身または子孫から名前でGameObjectを探す。
GameObject* FindDescendantByName(GameObject* object, const std::string& name) {
	if (!object) {
		return nullptr;
	}
	if (object->GetName() == name) {
		return object;
	}
	for (GameObject* child : object->GetChildren()) {
		if (GameObject* found = FindDescendantByName(child, name)) {
			return found;
		}
	}
	return nullptr;
}

// 両端がなめらかに繋がる補間。振り上げ/復帰の緩急に使う。
float SmoothStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// 手前が速く終わりが緩む。踏み下ろしの加速感に使う。
float EaseInQuad(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t;
}

} // namespace

void GuardianBossComponent::Initialize() {
	// BTのアクション登録。FunctionCatalogへ同時登録するとBahamutAIEditorの一覧に反映される。
	BahamutAI::FunctionCatalog catalog;

	// --- Condition ---

	BahamutAI::RegisterCondition(
	    btFactory_, catalog,
	    BahamutAI::ConditionDef("IsTargetWithin").Category("Sense").Description("対象との水平距離がdistance以内か").Float("distance", {8.0f}, "判定距離(m)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
		    (void)context;
		    return IsTargetWithin(params);
	    });

	// --- Movement ---

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("MoveToTarget")
	        .Category("Movement")
	        .Description("speed分だけ対象へ向かって進む。speedを負にすると後退する(毎TickSuccess)")
	        .Float("speed", {3.0f}, "移動速度(m/s)。負で後退")
	        .Float("turnSpeed", {2.0f}, "旋回速度(rad/s)。0で旋回しない"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return MoveToTarget(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("FaceTarget").Category("Movement").Description("対象の方向へ旋回する。向き切るまでRunning").Float("turnSpeed", {2.0f}, "旋回速度(rad/s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return FaceTarget(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("BodySpin")
	        .Category("Movement")
	        .Description("胴体(球体)だけを回す。脚は接地したまま")
	        .Float("duration", {1.5f}, "回す長さ(s)")
	        .Float("speed", {360.0f}, "回転速度(deg/s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return BodySpin(context, params); });

	// --- 足の攻撃。Sequenceで組み合わせて1つの攻撃を構成する ---

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("StompRaise")
	        .Category("Attack")
	        .Description("脚を1本選んで振り上げる。曲線レイヤーへ主導権を移すのでその脚は歩行を止める")
	        .Float("duration", {0.5f}, "振り上げにかける時間(s)")
	        .Float("hold", {0.0f}, "振り上げ切った頂点で静止する時間(s)。**遅延攻撃はここで作る**")
	        .Float("leg", {-1.0f}, "使う脚の番号(0〜3)。-1で対象に最も向いている脚を自動選択")
	        .Float("height", {2.2f}, "振り上げる高さ(m)")
	        .Float("forward", {0.6f}, "振り上げ時に対象側へ寄せる量(m)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return StompRaise(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("StompSlam")
	        .Category("Attack")
	        .Description("対象へ向けて踏み下ろす。攻撃判定ON")
	        .Float("duration", {0.22f}, "フェーズの長さ(s)。短いほど鋭い")
	        .Float("reach", {3.2f}, "ルート中心から踏み込める最大の水平距離(m)")
	        .Float("depth", {0.0f}, "着弾点の高さ(ルート基準)。負で地面へめり込ませる")
	        .Float("waveRadius", {4.0f}, "衝撃刃が広がり切ったときの半径(m)。足そのものより広く取ると当てやすい")
	        .Float("waveStart", {1.0f}, "衝撃刃の開始半径(m)")
	        .Float("waveDuration", {0.35f}, "衝撃刃が広がって消えるまでの時間(s)")
	        .Float("waveDamage", {14.0f}, "衝撃刃のダメージ")
	        .Float("waveKnockback", {9.0f}, "衝撃刃の吹き飛ばし初速")
	        .Float("waveStun", {0.6f}, "衝撃刃で動けなくなる時間(s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return StompSlam(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("StompRecover")
	        .Category("Attack")
	        .Description("脚を歩行へ返す。曲線レイヤーの主導権を戻すと歩行側が自動で踏み直す")
	        .Float("duration", {0.45f}, "フェーズの長さ(s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return StompRecover(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("LegSweep")
	        .Category("Attack")
	        .Description("振り上げた脚を横へ薙ぎ払う。攻撃判定ON。StompRaiseの後に繋ぐ")
	        .Float("duration", {0.5f}, "フェーズの長さ(s)")
	        .Float("radius", {3.0f}, "薙ぎ払う円弧の半径(m)")
	        .Float("arc", {150.0f}, "薙ぎ払う角度(deg)")
	        .Float("height", {0.9f}, "薙ぎ払う高さ(ルート基準, m)")
	        .Float("waveRadius", {4.5f}, "衝撃刃が広がり切ったときの半径(m)。足そのものより広く取ると当てやすい")
	        .Float("waveStart", {1.5f}, "衝撃刃の開始半径(m)")
	        .Float("waveDuration", {0.3f}, "衝撃刃が広がって消えるまでの時間(s)")
	        .Float("waveDamage", {12.0f}, "衝撃刃のダメージ")
	        .Float("waveKnockback", {11.0f}, "衝撃刃の吹き飛ばし初速")
	        .Float("waveStun", {0.6f}, "衝撃刃で動けなくなる時間(s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return LegSweep(context, params); });

	// --- 飛びかかり。遠距離から一気に詰めて全脚で踏み潰す ---

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("LeapCharge")
	        .Category("Leap")
	        .Description("飛びかかりの溜め。沈み込みつつ対象を向き、着地点を決める")
	        .Float("duration", {0.7f}, "溜めの長さ(s)")
	        .Float("sink", {0.7f}, "沈み込む深さ(m)")
	        .Float("turnSpeed", {3.0f}, "旋回速度(rad/s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return LeapCharge(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("LeapFly")
	        .Category("Leap")
	        .Description("放物線で着地点まで飛ぶ。4本とも脚を畳む")
	        .Float("duration", {0.85f}, "滞空時間(s)")
	        .Float("height", {6.0f}, "跳躍の高さ(m)")
	        .Float("range", {16.0f}, "跳べる最大の水平距離(m)")
	        .Float("stopDistance", {2.4f}, "対象の手前で止まる距離(m)。0だと真上へ落ちる")
	        .Float("tuck", {0.45f}, "脚を内側へ引き寄せる割合(0〜1)")
	        .Float("lift", {1.2f}, "脚を持ち上げる高さ(m)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return LeapFly(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("LeapSlam")
	        .Category("Leap")
	        .Description("着地。4本すべてを外へ叩きつける。全脚の攻撃判定ON")
	        .Float("duration", {0.18f}, "叩きつけの長さ(s)。短いほど鋭い")
	        .Float("spread", {3.6f}, "叩きつける水平距離(m)")
	        .Float("depth", {-0.1f}, "叩きつける高さ(ルート基準, m)。負で地面へめり込む")
	        .Float("waveRadius", {6.0f}, "衝撃刃が広がり切ったときの半径(m)。足そのものより広く取ると当てやすい")
	        .Float("waveStart", {1.5f}, "衝撃刃の開始半径(m)")
	        .Float("waveDuration", {0.4f}, "衝撃刃が広がって消えるまでの時間(s)")
	        .Float("waveDamage", {18.0f}, "衝撃刃のダメージ")
	        .Float("waveKnockback", {12.0f}, "衝撃刃の吹き飛ばし初速")
	        .Float("waveStun", {0.6f}, "衝撃刃で動けなくなる時間(s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return LeapSlam(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("LeapRecover").Category("Leap").Description("全脚を歩行へ返す").Float("duration", {0.5f}, "フェーズの長さ(s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return LeapRecover(context, params); });

	// --- ジェット飛行。胴体下部のエンジンで低空を浮遊し、脚を高速回転させて追い回す ---

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("JetWarn")
	        .Category("Jet")
	        .Description("コマ回転の予兆。沈み込んで脚を抱え込み、胴体を点滅させながらゆっくり回り始める(判定なし)")
	        .Float("duration", {1.2f}, "予兆の長さ(s)。長いほど避ける猶予が増える")
	        .Float("sink", {0.6f}, "沈み込む量(m)")
	        .Float("tuck", {0.35f}, "脚を内側へ引き寄せる割合(0〜1)")
	        .Float("lift", {0.5f}, "脚を持ち上げる高さ(m)")
	        .Float("spinSpeed", {240.0f}, "予兆中の回転速度(deg/s)。本番より遅くして「溜め」に見せる")
	        .Float("flashHz", {6.0f}, "胴体の点滅の速さ(回/秒)")
	        .Float("flashIntensity", {8.0f}, "点滅の明るさ"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return JetWarn(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("JetLiftOff")
	        .Category("Jet")
	        .Description("コマの構えへ移る。脚を外へ突っ張らせ、胴体ごと回し始める")
	        .Float("duration", {0.6f}, "構えに移る時間(s)")
	        .Float("height", {-0.7f}, "地面からルートまでの高さ(m)。**負にすると胴体が地面に接地する**")
	        .Float("spinReach", {1.0f}, "脚を伸ばす割合(最大長に対する。1.0でピンと伸ばし切る)")
	        .Float("spinPitch", {0.0f}, "脚の傾き(deg)。0で地面と平行")
	        .Float("spinSpeed", {540.0f}, "**胴体ごと**回る速度(deg/s)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return JetLiftOff(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("JetChase")
	        .Category("Jet")
	        .Description("コマのまま対象へ寄っていく。回転中は全脚の攻撃判定ON")
	        .Float("duration", {5.0f}, "回り続ける時間(s)")
	        .Float("height", {-0.7f}, "地面からルートまでの高さ(m)。**負にすると胴体が地面に接地する**")
	        .Float("speed", {2.5f}, "接近速度(m/s)。向きは変えず位置だけ寄せる。速すぎると当たる前に通り過ぎる")
	        .Float("spinReach", {1.0f}, "脚を伸ばす割合(最大長に対する。1.0でピンと伸ばし切る)")
	        .Float("spinPitch", {0.0f}, "脚の傾き(deg)。0で地面と平行")
	        .Float("spinSpeed", {900.0f}, "**胴体ごと**回る速度(deg/s)")
	        .Float("strikeScale", {2.0f}, "回転中だけ脚の攻撃判定を太らせる倍率。高速で振り回すとすり抜けるため")
	        .Float("waveRadius", {4.5f}, "**掃いている円盤そのものの判定**の半径(m)。脚の先端だけでは輪の内側に入られると当たらない")
	        .Float("waveHeight", {0.8f}, "円盤の判定の高さ(地面から, m)")
	        .Float("waveDamage", {16.0f}, "轢いたときのダメージ")
	        .Float("waveKnockback", {12.0f}, "轢いたときの吹き飛ばし初速")
	        .Float("waveStun", {0.7f}, "轢かれて動けなくなる時間(s)")
	        .Float("waveHitInterval", {0.7f}, "同じ相手へ轢き直すまでの間隔(s)。短いと乗られただけで即死する"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return JetChase(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("JetLand")
	        .Category("Jet")
	        .Description("回転を落として着地し、脚を歩行へ返す")
	        .Float("duration", {0.6f}, "着地にかける時間(s)")
	        .Float("spinReach", {1.0f}, "脚を伸ばす割合(最大長に対する)")
	        .Float("spinPitch", {0.0f}, "脚の傾き(deg)。0で地面と平行")
	        .Float("spinSpeed", {360.0f}, "着地までの回転速度(deg/s)。落としていくと止まって見える"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return JetLand(context, params); });

	// --- ビーム。脚を使わない遠距離攻撃。溜め→照射→消灯の3段 ---

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("BeamCharge")
	        .Category("Beam")
	        .Description("ビームの予告。**点滅**しながら対象を向く。当たり判定は出さない")
	        .Float("duration", {1.2f}, "点滅で予告する長さ(s)。ここが短いと理不尽な攻撃になる")
	        .Float("blinkInterval", {0.12f}, "点滅の間隔(s)。小さいほど速く瞬く")
	        .Float("turnSpeed", {2.5f}, "旋回速度(rad/s)。予告中に正面を取り切る")
	        .Float("length", {14.0f}, "ビームの長さ(ルート基準ローカル)。ルートのscaleが2なら見た目は倍")
	        .Float("thickness", {0.35f}, "点滅させる太さ。照射時と同じにすると「点滅か実体か」だけの差になる")
	        .Float("aimHeight", {1.0f}, "対象のどの高さを狙うか(m)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return BeamCharge(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("BeamFire")
	        .Category("Beam")
	        .Description("ビームを照射する。攻撃判定ON。turnSpeedを落とすと薙ぎ払いになり、避ける余地が生まれる")
	        .Float("duration", {1.0f}, "照射し続ける時間(s)")
	        .Float("turnSpeed", {0.8f}, "旋回速度(rad/s)。**ここが避けられるかどうかを決める**")
	        .Float("length", {14.0f}, "ビームの長さ(ルート基準ローカル)")
	        .Float("thickness", {0.35f}, "ビームの太さ")
	        .Float("aimHeight", {1.0f}, "対象のどの高さを狙うか(m)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return BeamFire(context, params); });

	BahamutAI::RegisterAction(
	    btFactory_, catalog,
	    BahamutAI::ActionDef("BeamRecover")
	        .Category("Beam")
	        .Description("ビームを細めて消す。攻撃判定OFF")
	        .Float("duration", {0.15f}, "消えるまでの時間(s)。短いほど切れ味が出る")
	        .Float("length", {14.0f}, "消える間のビームの長さ(ルート基準ローカル)")
	        .Float("thickness", {0.35f}, "消え始めの太さ。BeamFireと同じ値にする")
	        .Float("aimHeight", {1.0f}, "対象のどの高さを狙うか(m)"),
	    [this](BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) { return BeamRecover(context, params); });

	catalog.SaveToBTSetFolder(BTSetFolder());
}

void GuardianBossComponent::OnPlayStart() {
	LoadBTSet();

	if (!btObserver_) {
		// **キーはツリー名と完全に一致させること。**
		// エディタは購読要求に「ツリー名のハッシュ」を載せて送ってくる。ここが違うと
		// UdpTreeObserver::ShouldTransmit() が常にfalseになり、監視をONにしても
		// 一切パケットが飛ばない(エラーも出ないので気づきにくい)。
		// GuardianBT/BehaviorTree.json の activeTreeName は "Guardian"。
		btObserver_ = std::make_unique<BahamutAI::UdpTreeObserver>("Guardian");
	}

	currentPhase_.clear();
	phaseTimer_ = 0.0f;
	stunElapsed_ = 0.0f;
	// 致命の仰け反りも必ず解く。演出の途中でPlayを止めると、次のPlayが
	// 仰け反ったまま始まってBTが一切回らなくなる(ボスが棒立ちになる)。
	criticalRecoilTimer_ = 0.0f;
	SetBodyPitchOffset(0.0f);
	// カメラが脚に反応しないよう、体ごと専用レイヤーへ隔離する(当たり判定は残る)。
	ApplyBodyPartLayer();
	// **エンジンの炎へ魂の色を流し込む。**
	// Prefabの色をそのまま使わないのは、プレイヤーの魂の炎と必ず同じ色にしたいから。
	// GameFx::kSoulColor の1箇所を変えれば、魂の炎もエンジンの炎も一緒に変わる。
	if (GameObject* flame = FindDescendantByName(owner_, "EngineFlame")) {
		if (ParticleSystemComponent* system = flame->GetComponent<ParticleSystemComponent>()) {
			system->SetColorOverride(GameFx::kSoulColor);
		}
	}

	flinchTimer_ = 0.0f;
	stunStartLocal_.clear();
	// 衝撃刃はPlayインスタンスごとに作り直す(前回Playのポインタは無効)。
	shockwave_ = nullptr;
	shockwaveTried_ = false;
	shockwaveTimer_ = 0.0f;
	shockwaveHold_ = false;
	AbortAttack();

	// 体勢崩し(スタン)とのけぞりはEnemyHealthから通知を受ける。
	health_ = GetComponent<EnemyHealth>();
	if (health_) {
		health_->SetOnStagger([this]() { OnStaggered(); });
		health_->SetOnStaggerEnd([this]() { OnStaggerEnd(); });
		health_->SetOnFlinch([this]() { OnFlinch(); });
		health_->SetOnCritical([this](float seconds) { OnCriticalReceived(seconds); });
	}
}

namespace {

// 自分と全ての子孫のレイヤーを揃える。
void SetLayerRecursive(GameObject* object, uint32_t layer) {
	if (!object) {
		return;
	}
	object->SetLayer(layer);
	for (GameObject* child : object->GetChildren()) {
		SetLayerRecursive(child, layer);
	}
}

} // namespace

void GuardianBossComponent::ApplyBodyPartLayer() {
	if (!owner_) {
		return;
	}
	uint32_t layer = static_cast<uint32_t>(std::clamp(bodyPartLayer_, 0, 31));
	SetLayerRecursive(owner_, layer);
}

void GuardianBossComponent::OnStaggered() {
	// 攻撃を中断し、実行中の分岐を捨てる(スタン明けに途中から再開しないように)。
	AbortAttack();
	if (btRuntime_.IsLoaded()) {
		btRuntime_.Reset();
	}
	flinchTimer_ = 0.0f;
	stunElapsed_ = 0.0f;

	// 今の足先位置から広げ位置へ補間するため、開始時の位置を覚える。
	stunStartLocal_.clear();
	if (IGuardianLegRig* rig = GetRig()) {
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			stunStartLocal_.push_back(ToRootLocal(rig->GetFootWorld(index)));
		}
	}
}

void GuardianBossComponent::OnStaggerEnd() {
	// 全脚を歩行へ返し(AbortAttackはactiveLeg_しか戻さない)、沈みも戻す。
	if (IGuardianLegRig* rig = GetRig()) {
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			rig->SetCurveWeight(index, 0.0f);
		}
	}
	AbortAttack();
	stunStartLocal_.clear();
}

void GuardianBossComponent::OnCriticalReceived(float recoilSeconds) {
	// **致命を受けたら攻撃を丸ごと畳む。** 仰け反っている最中に攻撃判定が残っていると、
	// 決めたはずの一撃で相討ちになる。
	StopShockwave();
	SetAllStrikesActive(false);
	btRuntime_.Reset();

	criticalRecoilTimer_ = (recoilSeconds > 0.0f) ? recoilSeconds : 1.5f;
	criticalRecoilDuration_ = criticalRecoilTimer_;
	// 通常ののけぞりは打ち消す(二重に沈まないように)。
	flinchTimer_ = 0.0f;
}

void GuardianBossComponent::UpdateCriticalRecoil(float deltaTime) {
	if (criticalRecoilTimer_ <= 0.0f) {
		return;
	}
	criticalRecoilTimer_ -= deltaTime;
	float remain = (std::max)(criticalRecoilTimer_, 0.0f);
	float t = 1.0f - remain / (std::max)(criticalRecoilDuration_, 1.0e-3f);

	// **前半で一気に反り返り、後半でゆっくり戻る。**
	// 対称な山形にすると「軽く揺れた」だけに見えるので、立ち上がりを鋭くして
	// 戻りを長く取り、巨体が持ち直すのに時間がかかっている、という見え方にする。
	float shape = (t < 0.25f) ? (t / 0.25f) : std::pow(1.0f - (t - 0.25f) / 0.75f, 1.6f);
	shape = std::clamp(shape, 0.0f, 1.0f);

	// 上を向いて反り返る(+Z前方の左手系ではX軸の負回転が上向き)。
	SetBodyPitchOffset(-criticalRecoilPitch_ * shape);
	SetBodySink(-criticalRecoilSink_ * shape);

	if (criticalRecoilTimer_ <= 0.0f) {
		criticalRecoilTimer_ = 0.0f;
		SetBodyPitchOffset(0.0f);
		SetBodySink(0.0f);
	}
}

void GuardianBossComponent::OnFlinch() {
	if (health_ && health_->IsStaggered()) {
		return;
	}
	AbortAttack();
	if (btRuntime_.IsLoaded()) {
		btRuntime_.Reset();
	}
	flinchTimer_ = flinchDuration_;
}

void GuardianBossComponent::UpdateStunPose(float deltaTime) {
	IGuardianLegRig* rig = GetRig();
	if (!rig || !health_) {
		return;
	}
	stunElapsed_ += deltaTime;

	float total = (std::max)(health_->GetStunDuration(), 1.0e-3f);
	float remaining = health_->GetStunRemaining();

	// 入り: 0.25秒で崩れ落ちる / 出: 最後の0.5秒で起き上がる(weightを戻して歩行へ引き継ぐ)。
	float collapse = SmoothStep(stunElapsed_ / 0.25f);
	float riseWindow = (std::min)(0.5f, total * 0.3f);
	float rise = remaining < riseWindow ? SmoothStep(1.0f - remaining / riseWindow) : 0.0f;

	int legCount = rig->GetLegCount();
	for (int index = 0; index < legCount; ++index) {
		Vector3 from = (index < static_cast<int>(stunStartLocal_.size())) ? stunStartLocal_[index] : rig->GetHomeLocal(index);
		Vector3 to = SpreadLocal(index, stunSpread_, 0.0f);
		rig->SetCurveWeight(index, 1.0f - rise);
		rig->SetCurveTargetLocal(index, GuardianRigMath::Lerp3(from, to, collapse));
	}

	// 胴体: 沈めて小さく震わせる。起き上がりで戻す。
	float tremble = stunTremble_ * std::sin(stunElapsed_ * 28.0f) * (1.0f - rise);
	SetBodySink((-stunSink_ * collapse + tremble) * (1.0f - rise));
}

void GuardianBossComponent::RegisterInvokableMethods(KujataEngine::InvokableMethodRegistry& registry) {
	registry.Add("LoadBTSet", [this]() { LoadBTSet(); });
}

void GuardianBossComponent::LoadBTSet() {
	if (!btRuntime_.LoadFromBTSetFolder(BTSetFolder(), btFactory_)) {
		const BahamutAI::BehaviorTreeLoadResult& result = btRuntime_.GetLastLoadResult();
		KujataEngine::Logger::Log(std::string("[GuardianBossComponent] BT load failed: ") + result.GetErrorMessage());
	}
}

void GuardianBossComponent::Update() {
	if (!owner_ || !btRuntime_.IsLoaded()) {
		return;
	}

	float deltaTime = Time::GetDeltaTime();

	// 衝撃刃はBTの状態に関わらず進める(スタンで攻撃が中断されても、出ている刃は自分で畳む)。
	UpdateShockwave(deltaTime);

	// スタン中: BTを回さず、スタン姿勢だけを作る。
	if (health_ && health_->IsStaggered()) {
		UpdateStunPose(deltaTime);
		return;
	}

	// のけぞり中: 胴体を一瞬沈めて戻すだけ。行動はしない。
	// **致命の仰け反り中はBTを回さない。** 動き出すと演出の絵が壊れる。
	if (criticalRecoilTimer_ > 0.0f) {
		UpdateCriticalRecoil(deltaTime);
		return;
	}

	if (flinchTimer_ > 0.0f) {
		flinchTimer_ -= deltaTime;
		float t = std::clamp(1.0f - flinchTimer_ / (std::max)(flinchDuration_, 1.0e-3f), 0.0f, 1.0f);
		// 0→1で「沈んで戻る」山形。
		float dip = std::sin(t * std::numbers::pi_v<float>);
		SetBodySink(-flinchSink_ * dip);
		if (flinchTimer_ <= 0.0f) {
			flinchTimer_ = 0.0f;
			SetBodySink(0.0f);
		}
		return;
	}

	BahamutAI::AIContext context{localBlackboard_};
	context.deltaTime = Time::GetDeltaTime();
	context.SetOwner(*owner_);
	context.observer = btObserver_.get();

	btRuntime_.Tick(context);
}

// ---------------------------------------------------------------------------
// BT Condition
// ---------------------------------------------------------------------------

bool GuardianBossComponent::IsTargetWithin(const BahamutAI::NodeParams& params) {
	GameObject* target = FindTarget();
	if (!owner_ || !target) {
		return false;
	}

	float distance = params.GetFloat("distance", 8.0f);
	Vector3 toTarget = target->GetTransform().translation_ - owner_->GetTransform().translation_;
	toTarget.y = 0.0f;
	return Length(toTarget) <= distance;
}

// ---------------------------------------------------------------------------
// BT Actions: Movement
// ---------------------------------------------------------------------------

BahamutAI::BTStatus GuardianBossComponent::MoveToTarget(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	GameObject* target = FindTarget();
	if (!owner_ || !target) {
		return BahamutAI::BTStatus::Failure;
	}

	float turnSpeed = params.GetFloat("turnSpeed", 2.0f);
	if (turnSpeed > 0.0f) {
		RotateTowardsTarget(turnSpeed, context.deltaTime);
	}

	Vector3 toTarget = target->GetTransform().translation_ - owner_->GetTransform().translation_;
	toTarget.y = 0.0f;
	float distance = Length(toTarget);
	if (distance <= 0.0001f) {
		return BahamutAI::BTStatus::Success;
	}

	// speedが負なら向きはそのままに後退する。距離を取る行動はこれで作る。
	float speed = params.GetFloat("speed", 3.0f);
	owner_->GetTransform().translation_ += toTarget * (speed * context.deltaTime / distance);

	// 移動部分のみ。「どこまで近づいたら止まるか」はツリー側でIsTargetWithinと組む。
	return BahamutAI::BTStatus::Success;
}

BahamutAI::BTStatus GuardianBossComponent::FaceTarget(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	if (!owner_ || !FindTarget()) {
		return BahamutAI::BTStatus::Failure;
	}

	if (RotateTowardsTarget(params.GetFloat("turnSpeed", 2.0f), context.deltaTime)) {
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::BodySpin(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	GameObject* owner = GetOwner();
	if (!owner) {
		return BahamutAI::BTStatus::Failure;
	}

	// GuardianBodyがrotation_を毎フレーム丸ごと書き直すので、Transformを直接触っても次フレームで消える。
	// 必ずAddSpin経由で足すこと。
	GuardianBody* body = GetComponent<GuardianBody>();
	if (!body) {
		return BahamutAI::BTStatus::Failure;
	}

	float progress = 0.0f;
	bool finished = TickPhase("BodySpin", params.GetFloat("duration", 1.5f), context.deltaTime, progress);

	body->AddSpin(params.GetFloat("speed", 360.0f) * kDegreeToRadian * context.deltaTime);

	return finished ? BahamutAI::BTStatus::Success : BahamutAI::BTStatus::Running;
}

// ---------------------------------------------------------------------------
// BT Actions: 足の攻撃
// ---------------------------------------------------------------------------

BahamutAI::BTStatus GuardianBossComponent::StompRaise(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "StompRaise");
	if (starting) {
		int leg = PickStompLeg(static_cast<int>(params.GetFloat("leg", -1.0f)));
		if (leg < 0) {
			return BahamutAI::BTStatus::Failure;
		}
		activeLeg_ = leg;

		// 今その脚が接地している場所を起点にする。ここから浮かせるので足が飛ばない。
		attackStartLocal_ = ToRootLocal(rig->GetFootWorld(leg));

		// 振り上げ先。定位置の外向き成分を保ったまま、対象側へ寄せて持ち上げる。
		Vector3 home = rig->GetHomeLocal(leg);
		Vector3 forwardBias = {0.0f, 0.0f, params.GetFloat("forward", 0.6f)};
		attackRaisedLocal_ = home + forwardBias;
		attackRaisedLocal_.y += params.GetFloat("height", 2.2f);

		rig->SetCurveTargetLocal(leg, attackStartLocal_);
	}

	if (activeLeg_ < 0) {
		return BahamutAI::BTStatus::Failure;
	}

	float riseDuration = (std::max)(params.GetFloat("duration", 0.5f), 1.0e-3f);
	float hold = (std::max)(params.GetFloat("hold", 0.0f), 0.0f);

	float progress = 0.0f;
	bool finished = TickPhase("StompRaise", riseDuration + hold, context.deltaTime, progress);

	// 振り上げ切ってから hold 秒だけ頂点で静止する。
	// **「ゆっくり上げる」のではなく「速く上げて止める」のが遅延攻撃の見せ方。**
	// 前者は予備動作が間延びするだけだが、後者は振り下ろしのタイミングを外せる。
	float riseProgress = std::clamp(progress * (riseDuration + hold) / riseDuration, 0.0f, 1.0f);

	// weightを0→1へ持っていくと、その脚だけ歩行から曲線制御へ滑らかに移る。
	float eased = SmoothStep(riseProgress);
	rig->SetCurveWeight(activeLeg_, eased);
	rig->SetCurveTargetLocal(activeLeg_, GuardianRigMath::Lerp3(attackStartLocal_, attackRaisedLocal_, eased));

	return finished ? BahamutAI::BTStatus::Success : BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::StompSlam(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig || activeLeg_ < 0) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "StompSlam");
	if (starting) {
		// 着弾点はフェーズ開始時の対象位置で固定する。追尾し続けると避けられなくなるため。
		Vector3 impact = attackRaisedLocal_;
		if (GameObject* target = FindTarget()) {
			Vector3 targetLocal = ToRootLocal(target->GetTransform().translation_);
			targetLocal.y = 0.0f;

			float reach = params.GetFloat("reach", 3.2f);
			float horizontal = std::sqrt(targetLocal.x * targetLocal.x + targetLocal.z * targetLocal.z);
			if (horizontal > reach && horizontal > 0.0001f) {
				targetLocal = targetLocal * (reach / horizontal);
			}
			impact = targetLocal;
		}
		impact.y = params.GetFloat("depth", 0.0f);
		attackImpactLocal_ = impact;

		SetStrikeActive(activeLeg_, true);
	}

	float progress = 0.0f;
	bool finished = TickPhase("StompSlam", params.GetFloat("duration", 0.22f), context.deltaTime, progress);

	// 加速しながら落とす。等速だと踏みつけの重さが出ない。
	float eased = EaseInQuad(progress);
	rig->SetCurveWeight(activeLeg_, 1.0f);
	rig->SetCurveTargetLocal(activeLeg_, GuardianRigMath::Lerp3(attackRaisedLocal_, attackImpactLocal_, eased));

	if (finished) {
		SetStrikeActive(activeLeg_, false);
		// 踏み抜いた地点から衝撃刃を広げる。足の判定は先端の球しかなく点でしか当たらないので、
		// 「踏まれた場所の周り」を面で拾うのがこの刃の役目。
		Vector3 impactWorld = GuardianRigMath::ComputeWorldPose(GetOwner()).TransformPoint(attackImpactLocal_);
		impactWorld.y = SampleGroundUnderRoot();
		StartShockwave(impactWorld, params.GetFloat("waveStart", 1.0f), params.GetFloat("waveRadius", 4.0f),
		    params.GetFloat("waveDuration", 0.35f), false, params.GetFloat("waveDamage", 14.0f),
		    params.GetFloat("waveKnockback", 9.0f), params.GetFloat("waveStun", 0.6f), 0.5f);
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::LegSweep(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig || activeLeg_ < 0) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "LegSweep");
	if (starting) {
		SetStrikeActive(activeLeg_, true);
	}

	float progress = 0.0f;
	bool finished = TickPhase("LegSweep", params.GetFloat("duration", 0.5f), context.deltaTime, progress);

	// 振り上げ位置の角度を始点として、arc度ぶん水平に薙ぐ。
	float radius = params.GetFloat("radius", 3.0f);
	float arc = params.GetFloat("arc", 150.0f) * kDegreeToRadian;
	float startAngle = std::atan2(attackRaisedLocal_.x, attackRaisedLocal_.z);
	float angle = startAngle - arc * 0.5f + arc * SmoothStep(progress);

	Vector3 sweep = {std::sin(angle) * radius, params.GetFloat("height", 0.9f), std::cos(angle) * radius};
	rig->SetCurveWeight(activeLeg_, 1.0f);
	rig->SetCurveTargetLocal(activeLeg_, sweep);

	if (finished) {
		SetStrikeActive(activeLeg_, false);
		// 薙いだ弧をまとめて拾う衝撃刃。脚の通り道は広いのに判定は先端の球だけなので、
		// 薙ぎ終わりに足元から刃を広げて「薙ぎ払われた」範囲を成立させる。
		Vector3 sweepWorld = GuardianRigMath::ComputeWorldPose(GetOwner()).TransformPoint(sweep);
		sweepWorld.y = SampleGroundUnderRoot();
		StartShockwave(sweepWorld, params.GetFloat("waveStart", 1.5f), params.GetFloat("waveRadius", 4.5f),
		    params.GetFloat("waveDuration", 0.3f), false, params.GetFloat("waveDamage", 12.0f),
		    params.GetFloat("waveKnockback", 11.0f), params.GetFloat("waveStun", 0.6f), 0.5f);
		// 復帰フェーズの起点を薙ぎ終わりに合わせる。
		attackRaisedLocal_ = sweep;
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::StompRecover(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig || activeLeg_ < 0) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "StompRecover");
	if (starting) {
		SetStrikeActive(activeLeg_, false);
	}

	float progress = 0.0f;
	bool finished = TickPhase("StompRecover", params.GetFloat("duration", 0.45f), context.deltaTime, progress);

	// weightを1→0へ戻すだけでよい。0.5を切った時点で歩行側が主導権を取り戻し、
	// 「曲線が運んだ先」を今いる場所として引き継いで踏み直してくれる。
	rig->SetCurveWeight(activeLeg_, 1.0f - SmoothStep(progress));

	if (finished) {
		rig->SetCurveWeight(activeLeg_, 0.0f);
		activeLeg_ = -1;
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

// ---------------------------------------------------------------------------
// BT Actions: 飛びかかり
// ---------------------------------------------------------------------------

BahamutAI::BTStatus GuardianBossComponent::LeapCharge(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	GameObject* target = FindTarget();
	if (!owner_ || !rig || !target) {
		return BahamutAI::BTStatus::Failure;
	}

	RotateTowardsTarget(params.GetFloat("turnSpeed", 3.0f), context.deltaTime);

	float progress = 0.0f;
	bool finished = TickPhase("LeapCharge", params.GetFloat("duration", 0.7f), context.deltaTime, progress);

	// 沈み込みは脚ではなく胴体を下げて表現する。脚は接地したままなので踏ん張って見える。
	SetBodySink(-params.GetFloat("sink", 0.7f) * SmoothStep(progress));

	if (finished) {
		// 踏み切りと着地点をここで確定する。以降は対象を追わない(避けられる攻撃にするため)。
		leapStartWorld_ = owner_->GetTransform().translation_;
		leapLandWorld_ = target->GetTransform().translation_;
	}
	return finished ? BahamutAI::BTStatus::Success : BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::LeapFly(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "LeapFly");
	if (starting) {
		// 着地点を跳べる距離と手前距離で丸める。
		Vector3 toLand = leapLandWorld_ - leapStartWorld_;
		toLand.y = 0.0f;
		float distance = Length(toLand);

		float stopDistance = params.GetFloat("stopDistance", 2.4f);
		float range = params.GetFloat("range", 16.0f);
		float wanted = std::clamp(distance - stopDistance, 0.0f, range);

		Vector3 direction = GuardianRigMath::SafeNormalize(toLand, {0.0f, 0.0f, 1.0f});
		Vector3 land = leapStartWorld_ + direction * wanted;

		// 着地点の地面高さへ合わせる。歩行と同じレイキャスト設定を使う。
		if (GuardianGait* gait = GetComponent<GuardianGait>()) {
			land.y = gait->SampleGroundAt(land);
		} else {
			land.y = leapStartWorld_.y;
		}
		leapLandWorld_ = land;

		// 4本とも曲線制御へ移す。歩行側はこの間その脚に触らない。
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			rig->SetCurveTargetLocal(index, ToRootLocal(rig->GetFootWorld(index)));
		}
		SetBodySink(0.0f);
	}

	float progress = 0.0f;
	bool finished = TickPhase("LeapFly", params.GetFloat("duration", 0.85f), context.deltaTime, progress);

	// 水平は等速、垂直は放物線。half-sineだと着地が緩むので、正弦の山で素直に上げ下げする。
	Vector3 position = GuardianRigMath::Lerp3(leapStartWorld_, leapLandWorld_, progress);
	position.y += std::sin(std::numbers::pi_v<float> * progress) * params.GetFloat("height", 6.0f);
	owner_->GetTransform().translation_ = position;

	// 脚を畳む。滞空の前半で畳み切って、後半は着地に備えて構えたままにする。
	float tuckBlend = SmoothStep(std::clamp(progress * 2.0f, 0.0f, 1.0f));
	float pull = params.GetFloat("tuck", 0.45f);
	float lift = params.GetFloat("lift", 1.2f);
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		rig->SetCurveWeight(index, 1.0f);
		Vector3 current = rig->GetCurveTargetLocal(index);
		Vector3 tucked = TuckedLocal(index, pull, lift);
		rig->SetCurveTargetLocal(index, GuardianRigMath::Lerp3(current, tucked, tuckBlend * 0.35f));
	}

	if (finished) {
		owner_->GetTransform().translation_ = leapLandWorld_;
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::LeapSlam(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "LeapSlam");
	if (starting) {
		SetAllStrikesActive(true);
	}

	float progress = 0.0f;
	bool finished = TickPhase("LeapSlam", params.GetFloat("duration", 0.18f), context.deltaTime, progress);

	// 畳んだ位置から一気に外へ叩きつける。加速させて重さを出す。
	float eased = EaseInQuad(progress);
	float spread = params.GetFloat("spread", 3.6f);
	float depth = params.GetFloat("depth", -0.1f);
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		Vector3 from = TuckedLocal(index, params.GetFloat("tuck", 0.45f), params.GetFloat("lift", 1.2f));
		Vector3 to = SpreadLocal(index, spread, depth);
		rig->SetCurveWeight(index, 1.0f);
		rig->SetCurveTargetLocal(index, GuardianRigMath::Lerp3(from, to, eased));
	}

	if (finished) {
		SetAllStrikesActive(false);
		// 着地の踏み潰しは全周へ。4本の脚の間をすり抜けられないようにする。
		Vector3 landWorld = owner_->GetTransform().translation_;
		landWorld.y = SampleGroundUnderRoot();
		StartShockwave(landWorld, params.GetFloat("waveStart", 1.5f), params.GetFloat("waveRadius", 6.0f),
		    params.GetFloat("waveDuration", 0.4f), false, params.GetFloat("waveDamage", 18.0f),
		    params.GetFloat("waveKnockback", 12.0f), params.GetFloat("waveStun", 0.8f), 0.5f);
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::LeapRecover(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "LeapRecover");
	if (starting) {
		SetAllStrikesActive(false);
		SetBodySink(0.0f);
	}

	float progress = 0.0f;
	bool finished = TickPhase("LeapRecover", params.GetFloat("duration", 0.5f), context.deltaTime, progress);

	// weightを戻すだけで、歩行側が「曲線が運んだ先」を引き継いで踏み直す。
	float weight = 1.0f - SmoothStep(progress);
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		rig->SetCurveWeight(index, weight);
	}

	if (finished) {
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			rig->SetCurveWeight(index, 0.0f);
		}
		activeLeg_ = -1;
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

// ---------------------------------------------------------------------------
// BT Actions: ジェット飛行
// ---------------------------------------------------------------------------

BahamutAI::BTStatus GuardianBossComponent::JetWarn(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "JetWarn");
	if (starting) {
		// 予兆中は当たらない。「これから来る」とだけ伝える時間にする。
		SetAllStrikesActive(false);
		SetAllStrikeScales(1.0f);
	}

	float progress = 0.0f;
	bool finished = TickPhase("JetWarn", params.GetFloat("duration", 1.2f), context.deltaTime, progress);

	float eased = SmoothStep(progress);

	// 沈み込んで脚を抱え込む(バネを縮めるように見せる)。
	SetBodySink(-params.GetFloat("sink", 0.6f) * eased);
	float tuck = params.GetFloat("tuck", 0.35f);
	float lift = params.GetFloat("lift", 0.5f);
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		rig->SetCurveStraight(index, false);
		// weightを0から上げるので、歩行姿勢から抱え込み姿勢へ自然に混ざる。
		rig->SetCurveWeight(index, eased);
		rig->SetCurveTargetLocal(index, TuckedLocal(index, tuck, lift));
	}

	// ゆっくり回り始める(本番の高速回転への助走)。
	SpinRootYaw(params.GetFloat("spinSpeed", 240.0f) * eased, context.deltaTime);

	// 胴体を赤く点滅させる。終盤ほど速く見えるよう、明滅は経過時間で回す。
	float flashHz = params.GetFloat("flashHz", 6.0f);
	float pulse = 0.5f + 0.5f * std::sin(phaseTimer_ * flashHz * 2.0f * std::numbers::pi_v<float>);
	SetBodyEmissive(true, Vector3{1.0f, 0.15f, 0.05f}, params.GetFloat("flashIntensity", 8.0f) * pulse);

	if (finished) {
		// 発光は予兆の役目が終わった時点で消す(攻撃本体へ持ち込まない)。
		SetBodyEmissive(false, Vector3{0.0f, 0.0f, 0.0f}, 0.0f);
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::JetLiftOff(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "JetLiftOff");
	if (starting) {
		jetGroundY_ = SampleGroundUnderRoot();
		// 今その脚がある場所から回転姿勢へ移すので、いきなり飛ばない。
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			rig->SetCurveTargetLocal(index, ToRootLocal(rig->GetFootWorld(index)));
		}
	}

	float progress = 0.0f;
	bool finished = TickPhase("JetLiftOff", params.GetFloat("duration", 0.6f), context.deltaTime, progress);

	float eased = SmoothStep(progress);
	float height = params.GetFloat("height", -0.7f) * eased;
	owner_->GetTransform().translation_.y = jetGroundY_ + height;

	// 胴体ごと回す。離陸中は weight と一緒に徐々に効かせる。
	SpinRootYaw(params.GetFloat("spinSpeed", 540.0f) * eased, context.deltaTime);

	for (int index = 0; index < rig->GetLegCount(); ++index) {
		rig->SetCurveWeight(index, eased);
	}
	ApplyLegBlades(params.GetFloat("spinReach", 1.0f), params.GetFloat("spinPitch", 0.0f), true);

	return finished ? BahamutAI::BTStatus::Success : BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::JetChase(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "JetChase");
	// 回転する脚がそのまま刃になる。**毎Tick入れ直す** —
	// 開始時に一度だけONにする作りだと、途中で何かがOFFにしたときに気づけないため。
	SetAllStrikesActive(true);
	// 刃の先端は1フレームに1m近く進むので、判定を太らせないとプレイヤーをすり抜ける。
	SetAllStrikeScales(params.GetFloat("strikeScale", 2.0f));

	if (starting) {
		// **脚の先端だけでは当たらない。** 刃は接合部から外へ伸びているので、
		// 密着されるとプレイヤーが回転の輪の内側に入り、先端の球は永久にすり抜ける。
		// 掃いている円盤そのものを判定にするため、ボスに追従する一定サイズの刃を出す。
		Vector3 center = owner_->GetTransform().translation_;
		center.y = SampleGroundUnderRoot() + params.GetFloat("waveHeight", 0.8f);
		float bladeRadius = params.GetFloat("waveRadius", 4.5f);
		// duration<=0 = JetChaseが終わるまで出しっぱなし。
		StartShockwave(center, bladeRadius, bladeRadius, 0.0f, true, params.GetFloat("waveDamage", 16.0f),
		    params.GetFloat("waveKnockback", 12.0f), params.GetFloat("waveStun", 0.7f), params.GetFloat("waveHitInterval", 0.7f));
	}

	float progress = 0.0f;
	bool finished = TickPhase("JetChase", params.GetFloat("duration", 5.0f), context.deltaTime, progress);

	float height = params.GetFloat("height", -0.7f);

	// 追跡。**旋回はしない** — Yawはコマの回転が使っているので、向きを変えずに位置だけ寄せる。
	// 接地していないので歩幅の制約が無く、歩行より速くできる。
	if (GameObject* target = FindTarget()) {

		Vector3 toTarget = target->GetTransform().translation_ - owner_->GetTransform().translation_;
		toTarget.y = 0.0f;
		float distance = Length(toTarget);
		if (distance > 0.0001f) {
			owner_->GetTransform().translation_ += toTarget * (params.GetFloat("speed", 2.5f) * context.deltaTime / distance);
		}
	}

	// 移動先の地面へ高度を合わせ直す。起伏があってもホバー高度を保つ。
	jetGroundY_ = SampleGroundUnderRoot();
	owner_->GetTransform().translation_.y = jetGroundY_ + height;

	SpinRootYaw(params.GetFloat("spinSpeed", 900.0f), context.deltaTime);
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		rig->SetCurveWeight(index, 1.0f);
	}
	ApplyLegBlades(params.GetFloat("spinReach", 1.0f), params.GetFloat("spinPitch", 0.0f), true);

	if (finished) {
		SetAllStrikesActive(false);
		SetAllStrikeScales(1.0f);
		StopShockwave();
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::JetLand(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "JetLand");
	if (starting) {
		SetAllStrikesActive(false);
		// 開始時の高度を固定する。毎フレーム現在高度から取り直すと、
		// 減っていく高さに毎回eased率を掛けることになり指数的に落ちてしまう。
		jetGroundY_ = SampleGroundUnderRoot();
		jetLandStartHeight_ = owner_->GetTransform().translation_.y - jetGroundY_;
	}

	float progress = 0.0f;
	bool finished = TickPhase("JetLand", params.GetFloat("duration", 0.6f), context.deltaTime, progress);

	float eased = SmoothStep(progress);

	// 高度を0へ落としつつ、weightも0へ戻す。
	// weightが0.5を切った時点で歩行側が主導権を取り戻し、その場から踏み直す。
	jetGroundY_ = SampleGroundUnderRoot();
	float landedHeight = jetLandStartHeight_ * (1.0f - eased);
	owner_->GetTransform().translation_.y = jetGroundY_ + landedHeight;

	SpinRootYaw(params.GetFloat("spinSpeed", 360.0f) * (1.0f - eased), context.deltaTime);
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		rig->SetCurveWeight(index, 1.0f - eased);
	}
	ApplyLegBlades(params.GetFloat("spinReach", 1.0f), params.GetFloat("spinPitch", 0.0f), true);

	if (finished) {
		owner_->GetTransform().translation_.y = jetGroundY_;
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			rig->SetCurveWeight(index, 0.0f);
			// **直線モードは必ずここで解除する。**
			// 立てっぱなしにすると歩行も他の攻撃も棒のままになる
			// (curveStraight_ は curveWeight と無関係に効くため)。
			rig->SetCurveStraight(index, false);
		}
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// BT Actions: Beam
//
// 脚を一切触らないので、歩行(GuardianGait)と完全に独立している。
// 「攻撃 = 何かの姿を時間で動かす」という骨格は踏みつけと同じで、
// 動かす対象が脚の曲線ではなく1個の子オブジェクトになっただけ。
// ---------------------------------------------------------------------------

BahamutAI::BTStatus GuardianBossComponent::BeamCharge(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	GameObject* beam = GetBeamObject();
	if (!owner_ || !beam) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "BeamCharge");
	if (starting) {
		beam->SetActive(true);
		// 溜め中は当てない。ここは「来るぞ」と伝えるための時間で、当たると回避の意味が消える。
		SetBeamAttack(false);
	}

	// 溜めの間に正面を取り切る。ここで取り切っておくから、照射中はゆっくり追ってよくなる。
	RotateTowardsTarget(params.GetFloat("turnSpeed", 2.5f), context.deltaTime);

	float progress = 0.0f;
	bool finished = TickPhase("BeamCharge", params.GetFloat("duration", 1.2f), context.deltaTime, progress);

	// **点滅で予告する。** だんだん太くするより、点滅→実射のほうが「来る」瞬間がはっきりして迫力が出る。
	// 太さは照射時と同じにして、違いを「点滅しているか / 実体か」だけに絞るのがコツ。
	float blinkInterval = (std::max)(params.GetFloat("blinkInterval", 0.12f), 1.0e-3f);
	beam->SetActive((static_cast<int>(phaseTimer_ / blinkInterval) % 2) == 0);
	UpdateBeam(params.GetFloat("length", 14.0f), params.GetFloat("thickness", 0.35f), params.GetFloat("aimHeight", 1.0f));

	return finished ? BahamutAI::BTStatus::Success : BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::BeamFire(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	GameObject* beam = GetBeamObject();
	if (!owner_ || !beam) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "BeamFire");
	if (starting) {
		beam->SetActive(true);
		SetBeamAttack(true);
	}

	// **照射中はわざと遅く旋回する。** 速いと必ず当たる=避けようがない攻撃になる。
	// 「走り続ければ抜けられるが、止まれば焼かれる」の境目がこの数値。
	RotateTowardsTarget(params.GetFloat("turnSpeed", 0.8f), context.deltaTime);

	float progress = 0.0f;
	bool finished = TickPhase("BeamFire", params.GetFloat("duration", 1.0f), context.deltaTime, progress);

	UpdateBeam(params.GetFloat("length", 14.0f), params.GetFloat("thickness", 0.35f), params.GetFloat("aimHeight", 1.0f));

	if (finished) {
		SetBeamAttack(false);
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

BahamutAI::BTStatus GuardianBossComponent::BeamRecover(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params) {
	GameObject* beam = GetBeamObject();
	if (!owner_ || !beam) {
		return BahamutAI::BTStatus::Failure;
	}

	bool starting = (currentPhase_ != "BeamRecover");
	if (starting) {
		SetBeamAttack(false);
	}

	float progress = 0.0f;
	bool finished = TickPhase("BeamRecover", params.GetFloat("duration", 0.15f), context.deltaTime, progress);

	// 細めてから消す。判定はもう切れているので、ここは「終わった」と見せるためだけの時間。
	float thickness = std::lerp(params.GetFloat("thickness", 0.35f), 0.0f, SmoothStep(progress));
	UpdateBeam(params.GetFloat("length", 14.0f), thickness, params.GetFloat("aimHeight", 1.0f));

	if (finished) {
		HideBeam();
		return BahamutAI::BTStatus::Success;
	}
	return BahamutAI::BTStatus::Running;
}

KujataEngine::GameObject* GuardianBossComponent::GetBeamObject() { return owner_ ? FindDescendantByName(owner_, beamObjectName_) : nullptr; }

void GuardianBossComponent::UpdateBeam(float length, float thickness, float aimHeight) {
	GameObject* beam = GetBeamObject();
	if (!beam) {
		return;
	}

	// --- 狙う向き(ピッチ)を決める ---
	// ヨーはルートの旋回(RotateTowardsTarget)が担当し、ここはピッチだけ書く。
	// 役割を分けておくと「狙いの追従の遅さ」と「高さが合うか」を別々に調整できる。
	float pitch = 0.0f;
	if (GameObject* target = FindTarget()) {
		Vector3 aimWorld = target->GetTransform().translation_;
		aimWorld.y += aimHeight;

		Vector3 local = ToRootLocal(aimWorld);
		float horizontal = std::sqrt(local.x * local.x + local.z * local.z);
		// +Zを前とする左手系では、X軸まわりの正の回転が下を向く。だから符号を反転する。
		pitch = std::atan2(-(local.y - beamHeight_), (std::max)(horizontal, 1.0e-3f));
	}

	WorldTransform& transform = beam->GetTransform();
	transform.rotation_ = {pitch, 0.0f, 0.0f};

	// Cubeは原点が中心なので、長さLに伸ばすと根元がL/2だけ後ろへ突き抜ける。半分だけ前に出して根元を胴体に合わせる。
	// **前に出す向きはルートの+Zではなく「傾けた後の向き」**。ここを間違えるとピッチを付けた瞬間に根元が胴体から外れる。
	Vector3 forward = {0.0f, -std::sin(pitch), std::cos(pitch)};
	transform.scale_ = {thickness, thickness, length};
	transform.translation_ = Vector3{0.0f, beamHeight_, 0.0f} + forward * (length * 0.5f);
}

void GuardianBossComponent::SetBeamAttack(bool active) {
	if (GameObject* beam = GetBeamObject()) {
		if (EnemyWeapon* weapon = beam->GetComponent<EnemyWeapon>()) {
			weapon->SetAttack(active);
		}
	}
}

void GuardianBossComponent::HideBeam() {
	if (GameObject* beam = GetBeamObject()) {
		if (EnemyWeapon* weapon = beam->GetComponent<EnemyWeapon>()) {
			weapon->SetAttack(false);
		}
		beam->SetActive(false);
	}
}

void GuardianBossComponent::ApplyLegBlades(float reachRatio, float pitchDeg, bool straight) {
	IGuardianLegRig* rig = GetRig();
	if (!rig) {
		return;
	}

	float pitch = pitchDeg * kDegreeToRadian;

	for (int index = 0; index < rig->GetLegCount(); ++index) {
		// **脚は回さない。** 定位置(homeYaw)の向きのまま、自分の接合部からまっすぐ外へ突っ張らせるだけ。
		// 以前は脚ごとに角度をずらして回していたが、接合部の位置と脚の向きが噛み合わないため
		// 4本がバラバラの円を描いて形がいびつになった。回転はルートのYawに一本化する。
		Vector3 home = rig->GetHomeLocal(index);
		float yaw = std::atan2(home.x, home.z);

		Vector3 hipLocal = ToRootLocal(rig->GetHipWorld(index));
		Vector3 direction = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};

		// 最大長いっぱいまで伸ばすと弛みが消えて棒になる。pitch=0なら接合部と同じ高さ=地面と平行。
		float reach = rig->GetMaxReach(index) * std::clamp(reachRatio, 0.05f, 1.0f);

		rig->SetCurveStraight(index, straight);
		rig->SetCurveTargetLocal(index, hipLocal + direction * reach);
	}
}

void GuardianBossComponent::SpinRootYaw(float spinSpeedDeg, float deltaTime) {
	if (!owner_) {
		return;
	}

	// 脚の目標はルート基準のローカル座標で置いてあるので、ルートを回せば脚も胴体も一体で回る。
	// **旋回(RotateTowardsTarget)と同じ場所を書くので、回転中は対象を向かせないこと。**
	float& yaw = owner_->GetTransform().rotation_.y;
	yaw = WrapYawAngle(yaw + spinSpeedDeg * kDegreeToRadian * deltaTime);
}

float GuardianBossComponent::SampleGroundUnderRoot() const {
	GameObject* owner = GetOwner();
	if (!owner) {
		return 0.0f;
	}
	if (GuardianGait* gait = const_cast<GuardianBossComponent*>(this)->GetComponent<GuardianGait>()) {
		return gait->SampleGroundAt(owner->GetTransform().translation_);
	}
	return owner->GetTransform().translation_.y;
}

KujataEngine::Vector3 GuardianBossComponent::TuckedLocal(int legIndex, float pull, float lift) const {
	IGuardianLegRig* rig = const_cast<GuardianBossComponent*>(this)->GetRig();
	if (!rig) {
		return Vector3{0.0f, 0.0f, 0.0f};
	}
	// 定位置の向きは保ったまま内側へ引き寄せ、持ち上げる。向きを変えないので着地で素直に開ける。
	Vector3 home = rig->GetHomeLocal(legIndex);
	Vector3 tucked = home * std::clamp(1.0f - pull, 0.05f, 1.0f);
	tucked.y = lift;
	return tucked;
}

KujataEngine::Vector3 GuardianBossComponent::SpreadLocal(int legIndex, float distance, float height) const {
	IGuardianLegRig* rig = const_cast<GuardianBossComponent*>(this)->GetRig();
	if (!rig) {
		return Vector3{0.0f, 0.0f, 0.0f};
	}
	Vector3 home = rig->GetHomeLocal(legIndex);
	home.y = 0.0f;
	Vector3 direction = GuardianRigMath::SafeNormalize(home, {0.0f, 0.0f, 1.0f});
	Vector3 spread = direction * distance;
	spread.y = height;
	return spread;
}

void GuardianBossComponent::SetAllStrikesActive(bool active) {
	IGuardianLegRig* rig = GetRig();
	if (!rig) {
		return;
	}
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		SetStrikeActive(index, active);
	}
}

KujataEngine::GameObject* GuardianBossComponent::AcquireShockwave() {
	if (shockwave_ || shockwaveTried_) {
		return shockwave_;
	}
	shockwaveTried_ = true;

	Scene* scene = owner_ ? owner_->GetScene() : nullptr;
	if (!scene || shockwavePrefabPath_.empty()) {
		return nullptr;
	}
	// ランタイム生成なのでエディタのPrefab関連付けは不要(linkInstance=false)。
	PrefabAsset::InstantiateResult result = PrefabAsset::Instantiate(*scene, shockwavePrefabPath_, false);
	if (!result.succeeded || !result.rootObject) {
		Logger::Log("[GuardianBossComponent] shockwave prefab load failed (" + shockwavePrefabPath_ + "): " + result.message);
		return nullptr;
	}
	shockwave_ = result.rootObject;
	// 減衰の基準になる既定色を覚えておく(以後はこれにαの倍率を掛けて個体へ上書きする)。
	if (ModelRendererComponent* renderer = shockwave_->GetComponent<ModelRendererComponent>()) {
		shockwaveBaseColor_ = renderer->GetBaseColor();
	}
	shockwave_->SetActive(false);
	return shockwave_;
}

void GuardianBossComponent::StartShockwave(const Vector3& center, float startRadius, float endRadius, float duration, bool followOwner,
    float damage, float knockback, float stunDuration, float hitInterval) {
	GameObject* wave = AcquireShockwave();
	// **自分の脚や胴に貼らないよう、出し元を教える。**
	// 衝撃波は本体の子ではなくシーン直下に生成されるので、デカール側からは
	// 「誰が出したのか」が分からない。教えないとガーディアンの脚へ貼り付く。
	if (wave) {
		if (KujataEngine::DecalComponent* decal = wave->GetComponent<KujataEngine::DecalComponent>()) {
			decal->SetIgnoreRoot(owner_);
		}
	}
	// **土埃はここ1箇所で賄う。** ストンプもレッグスイープもリープ着地も回転刃も
	// 最後はこの関数を通るので、攻撃ごとに書くと必ずどれかを付け忘れる。
	// 強さは波の半径から決める。大きい攻撃ほど濃い土煙が上がる。
	if (owner_) {
		float strength = std::clamp(endRadius / 4.0f, 0.5f, 3.0f);
		GameFx::Burst(owner_->GetScene(), GameFx::Prefab::kDust, center, strength);
	}

	if (!wave) {
		return;
	}

	shockwaveCenter_ = center;
	shockwaveStartRadius_ = startRadius;
	shockwaveEndRadius_ = endRadius;
	shockwaveDuration_ = (std::max)(duration, 0.0f);
	shockwaveTimer_ = shockwaveDuration_;
	shockwaveFollow_ = followOwner;
	// durationが0以下なら「止めるまで出しっぱなし」。コマ回転のように長さをBT側が決める攻撃で使う。
	shockwaveHold_ = (duration <= 0.0f);

	if (EnemyWeapon* weapon = wave->GetComponent<EnemyWeapon>()) {
		weapon->SetHitParams(damage, knockback, stunDuration, hitInterval);
		// OFF→ONの立ち上がりでヒット履歴がクリアされるので、出すたびに当たり直せる。
		weapon->SetAttack(true);
	}

	WorldTransform& transform = wave->GetTransform();
	transform.translation_ = shockwaveCenter_;
	float radius = shockwaveStartRadius_;
	// Ringプリミティブは半径1のXZ円環なので、スケールがそのまま半径になる。
	// 当たりは球のままなので、輪の内側(通り過ぎた跡)にも判定は残る — 広がる波としてはこれで正しい。
	// **Transformのscaleは当たり判定(SphereCollider)を広げるために残す。**
	// 見た目はデカール側が世界座標で格子を組むので、このscaleの影響を受けない。
	transform.scale_ = {radius, 1.0f, radius};
	if (KujataEngine::DecalComponent* decal = shockwave_->GetComponent<KujataEngine::DecalComponent>()) {
		decal->SetRadius(radius);
	}
	ApplyShockwaveFade(1.0f);
	wave->SetActive(true);
}

void GuardianBossComponent::ApplyShockwaveFade(float alphaScale) {
	if (!shockwave_) {
		return;
	}
	// 色は基準色をそのまま使い、αだけを進捗で落とす。
	Vector4 color = shockwaveBaseColor_;
	color.w *= std::clamp(alphaScale, 0.0f, 1.0f);
	if (KujataEngine::DecalComponent* decal = shockwave_->GetComponent<KujataEngine::DecalComponent>()) {
		decal->SetColorOverride(color);
		return;
	}
	// 板ポリ構成のPrefabが残っていても動くよう、従来の経路も残しておく。
	if (ModelRendererComponent* renderer = shockwave_->GetComponent<ModelRendererComponent>()) {
		renderer->SetColorOverride(color);
	}
}

void GuardianBossComponent::StopShockwave() {
	shockwaveTimer_ = 0.0f;
	shockwaveHold_ = false;
	if (!shockwave_) {
		return;
	}
	if (EnemyWeapon* weapon = shockwave_->GetComponent<EnemyWeapon>()) {
		weapon->SetAttack(false);
	}
	shockwave_->SetActive(false);
}

void GuardianBossComponent::UpdateShockwave(float deltaTime) {
	if (!shockwave_ || !shockwave_->IsActive()) {
		return;
	}

	if (!shockwaveHold_) {
		shockwaveTimer_ -= deltaTime;
		if (shockwaveTimer_ <= 0.0f) {
			StopShockwave();
			return;
		}
	}

	// 経過に応じて広がる(hold中は常に終了半径のまま=一定の刃)。
	float t = 1.0f;
	if (!shockwaveHold_ && shockwaveDuration_ > 0.0f) {
		t = 1.0f - std::clamp(shockwaveTimer_ / shockwaveDuration_, 0.0f, 1.0f);
	}
	float radius = shockwaveStartRadius_ + (shockwaveEndRadius_ - shockwaveStartRadius_) * SmoothStep(t);

	WorldTransform& transform = shockwave_->GetTransform();
	if (shockwaveFollow_ && owner_) {
		// 追従する場合は高さだけ開始時の値を保つ(地面に沿って走る刃にする)。
		Vector3 position = owner_->GetTransform().translation_;
		position.y = shockwaveCenter_.y;
		transform.translation_ = position;
	}
	transform.scale_ = {radius, 1.0f, radius};

	// 広がるほど薄れて消える(土煙が散る)。出しっぱなしのコマ回転は濃さを保つ。
	ApplyShockwaveFade(shockwaveHold_ ? 1.0f : (1.0f - t));
}

void GuardianBossComponent::SetAllStrikeScales(float scale) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig) {
		return;
	}
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		std::string strikeName = "Leg" + std::to_string(index) + strikeObjectSuffix_;
		if (GameObject* strike = FindDescendantByName(owner_, strikeName)) {
			// SphereColliderComponentの半径はTransformのスケール(最大成分)に追従するので、
			// スケールを変えるだけで判定の太さが変わる。見た目は空のオブジェクトなので影響しない。
			strike->GetTransform().scale_ = {scale, scale, scale};
		}
	}
}

void GuardianBossComponent::SetBodyEmissive(bool active, const Vector3& color, float intensity) {
	IGuardianLegRig* rig = GetRig();
	GameObject* body = rig ? rig->GetBodyObject() : nullptr;
	if (!body) {
		return;
	}
	ModelRendererComponent* renderer = body->GetComponent<ModelRendererComponent>();
	if (!renderer) {
		return;
	}
	if (active) {
		renderer->SetEmissiveOverride(color, intensity);
	} else {
		renderer->ClearEmissiveOverride();
	}
}

void GuardianBossComponent::SetBodyPitchOffset(float radian) {
	if (GuardianBody* body = GetComponent<GuardianBody>()) {
		body->SetPitchOffset(radian);
	}
}

void GuardianBossComponent::SetBodySink(float offset) {
	if (GuardianBody* body = GetComponent<GuardianBody>()) {
		body->SetHeightOffset(offset);
	}
}

bool GuardianBossComponent::TickPhase(const char* phaseName, float duration, float deltaTime, float& outProgress) {
	if (currentPhase_ != phaseName) {
		currentPhase_ = phaseName;
		phaseTimer_ = 0.0f;
	}

	phaseTimer_ += deltaTime;

	float safeDuration = (std::max)(duration, 1.0e-3f);
	outProgress = std::clamp(phaseTimer_ / safeDuration, 0.0f, 1.0f);

	if (phaseTimer_ >= safeDuration) {
		currentPhase_.clear();
		phaseTimer_ = 0.0f;
		return true;
	}
	return false;
}

IGuardianLegRig* GuardianBossComponent::GetRig() { return GetComponent<IGuardianLegRig>(); }

KujataEngine::GameObject* GuardianBossComponent::FindTarget() {
	if (!owner_ || !owner_->GetScene()) {
		return nullptr;
	}

	// **ヘイト表があればそちらに従う。**
	// 最寄りを狙う方式だと、殴ってきた相手より単に近いだけの相手へ寄ってしまい、
	// キャラを切り替えて攻めても敵の狙いが変わらない。表が無い個体は従来どおり最寄りを狙う。
	if (HateTable* hate = GetComponent<HateTable>()) {
		if (GameObject* hated = hate->GetTarget()) {
			return hated;
		}
	}

	// targetTag_の付いた生存キャラのうち最寄りを狙う(HammerEnemyComponentと同じ方針)。
	GameObject* nearest = nullptr;
	float nearestDistSq = 0.0f;
	const Vector3 selfPosition = owner_->GetTransform().translation_;
	for (const auto& object : owner_->GetScene()->GetGameObjects()) {
		if (!object || !object->IsActiveInHierarchy() || object->GetTag() != targetTag_) {
			continue;
		}
		PlayerHealth* health = object->GetComponent<PlayerHealth>();
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

bool GuardianBossComponent::RotateTowardsTarget(float turnSpeed, float deltaTime) {
	GameObject* target = FindTarget();
	if (!owner_ || !target) {
		return false;
	}

	Vector3 toTarget = target->GetTransform().translation_ - owner_->GetTransform().translation_;
	toTarget.y = 0.0f;
	if (Length(toTarget) <= 0.0001f) {
		return true;
	}

	// +Z前方の左手系Yawはatan2(x, z)。
	float targetYaw = std::atan2(toTarget.x, toTarget.z);
	float currentYaw = owner_->GetTransform().rotation_.y;
	float difference = WrapYawAngle(targetYaw - currentYaw);

	float maxStep = turnSpeed * deltaTime;
	if (std::fabs(difference) <= maxStep) {
		owner_->GetTransform().rotation_.y = targetYaw;
		return true;
	}

	owner_->GetTransform().rotation_.y = WrapYawAngle(currentYaw + (difference > 0.0f ? maxStep : -maxStep));
	return false;
}

KujataEngine::Vector3 GuardianBossComponent::ToRootLocal(const KujataEngine::Vector3& worldPosition) const {
	return GuardianRigMath::ComputeWorldPose(GetOwner()).InverseTransformPoint(worldPosition);
}

int GuardianBossComponent::PickStompLeg(int requestedLeg) {
	IGuardianLegRig* rig = GetRig();
	if (!rig) {
		return -1;
	}
	if (requestedLeg >= 0 && requestedLeg < rig->GetLegCount()) {
		return requestedLeg;
	}

	GameObject* target = FindTarget();
	if (!target) {
		return 0;
	}

	// ルートローカルで見た対象方向と、各脚の定位置方向との内積が最大の脚を選ぶ。
	// ルート基準なので、胴体が回っていても選択がぶれない。
	Vector3 targetLocal = ToRootLocal(target->GetTransform().translation_);
	targetLocal.y = 0.0f;
	targetLocal = GuardianRigMath::SafeNormalize(targetLocal, {0.0f, 0.0f, 1.0f});

	int best = 0;
	float bestDot = -2.0f;
	for (int index = 0; index < rig->GetLegCount(); ++index) {
		Vector3 home = rig->GetHomeLocal(index);
		home.y = 0.0f;
		home = GuardianRigMath::SafeNormalize(home, {0.0f, 0.0f, 1.0f});

		float dot = GuardianRigMath::Dot3(home, targetLocal);
		if (dot > bestDot) {
			bestDot = dot;
			best = index;
		}
	}
	return best;
}

void GuardianBossComponent::SetStrikeActive(int legIndex, bool active) {
	IGuardianLegRig* rig = GetRig();
	if (!owner_ || !rig || legIndex < 0) {
		return;
	}

	// 脚の名前は "Leg0" 等。攻撃判定は "Leg0_Strike" に付いている想定。
	std::string strikeName = "Leg" + std::to_string(legIndex) + strikeObjectSuffix_;
	GameObject* strike = FindDescendantByName(owner_, strikeName);
	if (!strike) {
		return;
	}

	if (EnemyWeapon* weapon = strike->GetComponent<EnemyWeapon>()) {
		weapon->SetAttack(active);
	}
}

void GuardianBossComponent::AbortAttack() {
	IGuardianLegRig* rig = GetRig();
	if (rig) {
		if (activeLeg_ >= 0) {
			rig->SetCurveWeight(activeLeg_, 0.0f);
		}
		// 中断経路でも直線モードが残らないよう、全脚ぶん落としておく。
		// 曲線のまま歩くのが既定で、直線はジェット回転中だけの一時的な状態。
		for (int index = 0; index < rig->GetLegCount(); ++index) {
			rig->SetCurveStraight(index, false);
		}
	}
	SetAllStrikesActive(false);
	// 衝撃刃も一時的なものなので必ず消す。
	StopShockwave();
	// 判定の太らせと予兆の発光も、一時的にONにするものなので必ず戻す。
	SetAllStrikeScales(1.0f);
	SetBodyEmissive(false, Vector3{0.0f, 0.0f, 0.0f}, 0.0f);
	// 中断されても照射しっぱなしにならないよう、必ず消す。
	// 「一時的にONにするものは、中断経路でも必ずOFFへ戻す」— 直線モードで一度やらかしている。
	HideBeam();
	SetBodySink(0.0f);
	activeLeg_ = -1;
	currentPhase_.clear();
	phaseTimer_ = 0.0f;
}
