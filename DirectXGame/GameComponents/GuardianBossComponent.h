#pragma once

#include <BahamutAI/AI.h>
#include <KujataEngine.h>
#include <memory>
#include <string>

class IGuardianLegRig;

/// <summary>
/// ガーディアンの「頭脳」。BehaviorTree(BahamutAI)で行動を組み立てる。
/// ツリー本体はBahamutAIEditorで編集し、ここではCondition/Actionの実装と登録だけを行う。
/// 調整値はすべてBTノードのParamsで指定する(HammerEnemyComponentと同じ流儀)。
///
/// ノードは「動作単位」で切ってあり、ツリー側で組み替えて行動を作る。
///
/// 登録Condition:
///   IsTargetWithin  : 対象との水平距離が distance 以内か
///
/// 登録Action(移動):
///   MoveToTarget    : speed分だけ対象へ向かって進む。**speedを負にすると後退する**
///   FaceTarget      : 対象の方向へ旋回する
///   BodySpin        : 胴体(球体)だけを回す。脚は接地したまま
///
/// 登録Action(足の攻撃。Sequenceで 振り上げ→踏み下ろし→復帰 と繋いで1つの攻撃にする):
///   StompRaise      : 脚を1本選んで振り上げる(曲線レイヤーへ主導権を移す)
///   StompSlam       : 対象へ向けて踏み下ろす。攻撃判定ON
///   StompRecover    : 脚を歩行へ返す(曲線レイヤーの主導権を戻す)
///   LegSweep        : 振り上げた脚を横へ薙ぎ払う。攻撃判定ON
///
/// 登録Action(飛びかかり。遠距離から一気に詰めて全脚で踏み潰す):
///   LeapCharge      : 溜め。沈み込みつつ対象を向き、着地点を決める
///   LeapFly         : 放物線で着地点まで飛ぶ。4本とも脚を畳む
///   LeapSlam        : 着地。4本すべてを外へ叩きつける。全脚の攻撃判定ON
///   LeapRecover     : 全脚を歩行へ返す
///
/// 登録Action(コマ回転。胴体を地面に接地させ、脚を突っ張らせたまま body ごと高速回転する):
///   JetLiftOff      : コマの構えへ移る。脚を外へ突っ張らせ、胴体ごと回し始める
///   JetChase        : コマのまま対象へ寄っていく。**回転中は全脚の攻撃判定ON**
///   JetLand         : 回転を落として着地し、脚を歩行へ返す
///
/// 登録Action(ビーム。脚を使わない遠距離攻撃。子オブジェクト "Beam" を伸縮させるだけ):
///   BeamCharge      : 予告。点滅しながら正面を取る。判定は出さない
///   BeamFire        : 照射。攻撃判定ON。旋回を遅くすると薙ぎ払いになり避けられる
///   BeamRecover     : 細めて消す
///
/// 攻撃は GuardianSplineRig の曲線レイヤー(leg{i}.curveWeight / curveTarget)を直接動かす。
/// 歩行側(GuardianGait)は curveWeight が0.5を超えている間その脚の踏み出しを止め、
/// 戻った瞬間に踏み直すので、攻撃と歩行の受け渡しは自動で噛み合う。
/// </summary>
class GuardianBossComponent : public KujataEngine::Component {
public:
	const char* GetTypeName() const override { return "GuardianBossComponent"; }
	bool AllowMultiple() const override { return false; }

	void Initialize() override;
	void OnPlayStart() override;
	void Update() override;

	void RegisterInvokableMethods(KujataEngine::InvokableMethodRegistry& registry) override;

private:
	void LoadBTSet();

	// --- BT Conditions ---
	bool IsTargetWithin(const BahamutAI::NodeParams& params);

	// --- BT Actions ---
	BahamutAI::BTStatus MoveToTarget(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus FaceTarget(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus BodySpin(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus StompRaise(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus StompSlam(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus StompRecover(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus LegSweep(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus LeapCharge(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus LeapFly(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus LeapSlam(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus LeapRecover(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus JetLiftOff(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus JetChase(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus JetLand(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);

	// --- ビーム(遠距離)。脚を一切使わないので、歩行と完全に独立して撃てる ---
	BahamutAI::BTStatus BeamCharge(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus BeamFire(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
	BahamutAI::BTStatus BeamRecover(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);

	/// <summary>
	/// 脚を定位置の向きのまま、接合部からまっすぐ外へ突っ張らせた位置を全脚ぶん書き込みます。
	/// **脚自体は回しません。** 回すのはルートのYaw(SpinRootYaw)で、脚も胴体も一体で回るからコマになる。
	/// 脚ごとに角度をずらして回すと、接合部の位置と脚の向きが噛み合わず形がいびつになる。
	/// reachRatio=1.0 かつ straight=true で「ピンと伸ばした棒」、pitchDeg=0 で地面と平行。
	/// </summary>
	void ApplyLegBlades(float reachRatio, float pitchDeg, bool straight);

	/// <summary>
	/// ルート自身のYawを回します。脚の目標はルート基準で置いてあるので、
	/// ルートを回せば脚と胴体が一体で回る。**コマの回転はここで作る。**
	/// </summary>
	void SpinRootYaw(float spinSpeedDeg, float deltaTime);

	/// <summary>足元の地面の高さ。GuardianGaitが無ければ現在のルートYを返します。</summary>
	float SampleGroundUnderRoot() const;

	// --- helpers ---
	IGuardianLegRig* GetRig();

	/// <summary>全脚の攻撃判定をまとめてON/OFFします(着地の踏み潰し用)。</summary>
	void SetAllStrikesActive(bool active);

	/// <summary>胴体の高さオフセットを設定します(GuardianBodyが無ければ何もしない)。</summary>
	void SetBodySink(float offset);

	/// <summary>ビームの器(既定では "Beam" という子オブジェクト)を探します。</summary>
	KujataEngine::GameObject* GetBeamObject();

	/// <summary>
	/// ビームの向き・長さ・太さをまとめて更新します。ピッチだけをここで書き、ヨーはルートの旋回が担当する。
	/// Cubeは原点が中心なので、長さLにするには scale.z=L と「傾けた向きへ半分だけ前に出す」がセットで要る。
	/// BoxColliderのsizeが(1,1,1)なら当たり判定はscaleに自動追従するので、見た目と当たりがここだけで揃う。
	/// </summary>
	void UpdateBeam(float length, float thickness, float aimHeight);

	/// <summary>ビームのダメージ判定をON/OFFします。</summary>
	void SetBeamAttack(bool active);

	/// <summary>ビームを消します(判定OFF+非アクティブ化)。中断経路からも必ず通すこと。</summary>
	void HideBeam();

	/// <summary>脚を畳んだ位置(ルートローカル)。定位置の向きを保ったまま内側へ引き寄せて持ち上げる。</summary>
	KujataEngine::Vector3 TuckedLocal(int legIndex, float pull, float lift) const;

	/// <summary>脚を張り出した位置(ルートローカル)。定位置の向きのまま外側の指定の高さへ。</summary>
	KujataEngine::Vector3 SpreadLocal(int legIndex, float distance, float height) const;
	KujataEngine::GameObject* FindTarget();

	/// <summary>対象へYawだけ旋回する。ほぼ向いていればtrue。</summary>
	bool RotateTowardsTarget(float turnSpeed, float deltaTime);

	/// <summary>ワールド座標をGuardianルート基準のローカル座標へ変換します。</summary>
	KujataEngine::Vector3 ToRootLocal(const KujataEngine::Vector3& worldPosition) const;

	/// <summary>
	/// 対象へ最も向いている脚の番号を返します。paramのlegが0以上ならそれをそのまま使う。
	/// 定位置(ルートローカル)の向きと、ルートローカルでの対象方向との内積で選ぶ。
	/// </summary>
	int PickStompLeg(int requestedLeg);

	/// <summary>踏みつけに使う脚の攻撃判定(Leg*_Strike のEnemyWeapon)をON/OFFします。</summary>
	void SetStrikeActive(int legIndex, bool active);

	/// <summary>攻撃を中断して脚を歩行へ返します。</summary>
	void AbortAttack();

	/// <summary>
	/// フェーズ共通処理。前回と別フェーズなら開始扱いにし、経過時間を進めて
	/// 0〜1の進捗を outProgress へ返します。durationを超えたらtrue(=フェーズ完了)。
	/// </summary>
	bool TickPhase(const char* phaseName, float duration, float deltaTime, float& outProgress);

public:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		KUJATA_REGISTER_STRING_NAMED_TIP(targetTag_, "Target Tag",
		    "攻撃対象のタグ。このタグが付いた生存キャラ(PlayerHealth持ち)のうち最寄りを狙う。\n"
		    "名前ではなくタグで選ぶのは、プレイアブル2人+味方NPCの構成に対応するため。");
		KUJATA_REGISTER_STRING_NAMED_TIP(strikeObjectSuffix_, "Strike Object Suffix",
		    "足の攻撃判定オブジェクトの名前の接尾辞。\n"
		    "脚の名前(Leg0等)と繋げて \"Leg0_Strike\" を階層から探し、そのEnemyWeaponをON/OFFする。");
		KUJATA_REGISTER_STRING_NAMED_TIP(beamObjectName_, "Beam Object Name",
		    "ビームの器になる子オブジェクトの名前。\n"
		    "+Z方向へ伸びるCube + トリガーBoxCollider(size 1,1,1) + EnemyWeapon を持たせておく。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(beamHeight_, "Beam Height", 0.01f, 0.0f, 10.0f,
		    "ビームが出る高さ(ルート基準のローカル)。\n"
		    "GuardianBodyの Ride Height と同じ値にすると、胴体の中心から出ているように見える。");
	}

private:
	// 攻撃対象のタグ。
	KUJATA_FIELD_STRING(targetTag_, "Ally");
	// 足の攻撃判定オブジェクトの接尾辞。
	KUJATA_FIELD_STRING(strikeObjectSuffix_, "_Strike");
	// ビームの器になる子オブジェクトの名前。
	KUJATA_FIELD_STRING(beamObjectName_, "Beam");
	// ビームが出る高さ(ルート基準ローカル)。GuardianBodyのrideHeight_と揃えてある。
	KUJATA_FIELD_FLOAT(beamHeight_, 1.7f);

	// BT作成用。
	BahamutAI::BehaviorTreeFactory btFactory_;
	// BT実行本体(毎フレームTickする)。
	BahamutAI::BehaviorTreeRuntime btRuntime_;
	// ライブ監視オブザーバー。Playインスタンスだけが遅延生成する。
	std::unique_ptr<BahamutAI::UdpTreeObserver> btObserver_;
	// このボス固有のBlackboard。
	BahamutAI::Blackboard localBlackboard_;

	// --- 攻撃フェーズの実行状態 ---
	// 実行中フェーズ名(空=非攻撃中)。フェーズはRunningをまたぐので、開始済みかをこれで判定する。
	std::string currentPhase_;
	// 現在フェーズの経過時間[s]。
	float phaseTimer_ = 0.0f;
	// 攻撃に使っている脚(-1=なし)。
	int activeLeg_ = -1;
	// 攻撃開始時の足先(ルートローカル)。振り上げの起点。
	KujataEngine::Vector3 attackStartLocal_ = {0.0f, 0.0f, 0.0f};
	// 振り上げ切った位置(ルートローカル)。踏み下ろしの起点になる。
	KujataEngine::Vector3 attackRaisedLocal_ = {0.0f, 0.0f, 0.0f};
	// 踏み下ろし/薙ぎ払いの着弾点(ルートローカル)。フェーズ開始時に対象位置から決める。
	KujataEngine::Vector3 attackImpactLocal_ = {0.0f, 0.0f, 0.0f};

	// --- 飛びかかりの実行状態(こちらはワールド座標。ルート自体が動くため) ---
	// 踏み切り位置。
	KujataEngine::Vector3 leapStartWorld_ = {0.0f, 0.0f, 0.0f};
	// 着地位置。溜めの終わりに対象位置から決めて固定する。
	KujataEngine::Vector3 leapLandWorld_ = {0.0f, 0.0f, 0.0f};

	// --- ジェット飛行の実行状態 ---
	// 離陸を始めた高さ。降下の目標にも使う。
	float jetGroundY_ = 0.0f;
	// 降下開始時の高度。毎フレーム現在高度から計算し直すと指数的に落ちてしまうので固定する。
	float jetLandStartHeight_ = 0.0f;
};
