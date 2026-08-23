#pragma once

#include <BahamutAI/AI.h>
#include <KujataEngine.h>
#include <memory>
#include <string>
#include <vector>

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
///   JetWarn         : **予兆**。沈み込んで脚を抱え込み、胴体を赤く点滅させながらゆっくり回り始める。判定は出さない
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
	BahamutAI::BTStatus JetWarn(BahamutAI::AIContext& context, const BahamutAI::NodeParams& params);
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

	/// <summary>
	/// 自分と全ての子孫を Body Part Layer へ移します(OnPlayStartで1回)。
	/// **目的はカメラ**: OrbitCameraComponentは遮蔽物を探すときに Obstacle Mask でレイヤーを絞るので、
	/// ボスの体を専用レイヤーへ隔離し、カメラ側のマスクからそのビットを外せば
	/// 「胴体の下に潜り込んだとき、振り回される脚にカメラが反応してガクガクする」のを断てる。
	/// 当たり判定そのものは残るので、脚で殴られるし脚を斬ることもできる。
	///
	/// Prefabのフォーマットはlayerを保存しないため、**データではなくコードで毎回入れ直す**。
	/// </summary>
	void ApplyBodyPartLayer();

	/// <summary>足元の地面の高さ。GuardianGaitが無ければ現在のルートYを返します。</summary>
	float SampleGroundUnderRoot() const;

	// --- helpers ---
	IGuardianLegRig* GetRig();

	/// <summary>
	/// 衝撃刃(広い当たり判定の器)を出します。脚の先端の球だけでは「振り回した軌跡」を拾えないため、
	/// 踏みつけ・薙ぎ払い・着地・コマ回転はこの面の判定で当てる。
	/// Prefabからランタイム生成して使い回す(1体につき1つ)。
	///
	///   center      : 中心のワールド座標
	///   startRadius : 開始半径 / endRadius : 終了半径(広がる衝撃波なら start<end、一定なら同じ値)
	///   duration    : 出ている時間[s]。0以下なら明示的にStopShockwaveするまで出しっぱなし
	///   followOwner : trueなら毎フレーム中心をボスへ追従させる(コマ回転のように動きながら当てる場合)
	/// </summary>
	void StartShockwave(const KujataEngine::Vector3& center, float startRadius, float endRadius, float duration, bool followOwner,
	    float damage, float knockback, float stunDuration, float hitInterval);

	/// <summary>衝撃刃を消します。中断経路からも必ず通すこと。</summary>
	void StopShockwave();

	/// <summary>衝撃刃の広がりと寿命を進めます(毎フレーム呼ぶ)。</summary>
	void UpdateShockwave(float deltaTime);

	/// <summary>衝撃刃の器(Prefabから遅延生成)。失敗したらnullptr。</summary>
	KujataEngine::GameObject* AcquireShockwave();

	/// <summary>
	/// 衝撃刃の濃さ(0=透明, 1=Materialの既定値)。**Materialアセットではなく個体へ上書きする** —
	/// アセットを直接書き換えると、同時に出ている他の衝撃刃まで一緒に薄くなる。
	/// </summary>
	void ApplyShockwaveFade(float alphaScale);

	/// <summary>全脚の攻撃判定をまとめてON/OFFします(着地の踏み潰し用)。</summary>
	void SetAllStrikesActive(bool active);

	/// <summary>
	/// 脚の攻撃判定の大きさを倍率で変えます(SphereColliderの半径はTransformのスケールに追従する)。
	/// 高速で振り回す攻撃は判定がすり抜けやすいので、その間だけ太らせて確実に当てるために使う。
	/// **1.0へ戻す責任は呼び出し側にある**(AbortAttackでも必ず戻すこと)。
	/// </summary>
	void SetAllStrikeScales(float scale);

	/// <summary>
	/// 胴体を発光させます(予兆の点滅用)。activeがfalseなら元のマテリアルへ戻します。
	/// 一時的にONにするものなので、中断経路でも必ずOFFへ戻すこと。
	/// </summary>
	void SetBodyEmissive(bool active, const KujataEngine::Vector3& color, float intensity);

	/// <summary>胴体の高さオフセットを設定します(GuardianBodyが無ければ何もしない)。</summary>
	void SetBodySink(float offset);
	/// <summary>胴体へ追加のピッチ[rad]を与える(致命の仰け反り)。0で通常。</summary>
	void SetBodyPitchOffset(float radian);

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

	// --- 体勢崩し(スタン)・のけぞり。EnemyHealthのコールバックから呼ばれる ---

	/// <summary>スタン開始。攻撃を中断しBTの分岐を捨て、スタン姿勢(UpdateStunPose)へ移る。</summary>
	void OnStaggered();
	/// <summary>スタン終了。脚を歩行へ返す。</summary>
	void OnStaggerEnd();
	/// <summary>短いのけぞり(ジャストガード時)。攻撃を中断し、Flinch Duration秒だけ胴体を沈めて行動を止める。</summary>
	void OnFlinch();

	/// <summary>
	/// 致命を受けた。**攻撃を畳んで大きく仰け反る。**
	/// 通常ののけぞり(OnFlinch)より深く長く、その間BTを完全に止める。
	/// </summary>
	void OnCriticalReceived(float recoilSeconds);
	/// <summary>仰け反りの姿勢を毎フレーム進める。</summary>
	void UpdateCriticalRecoil(float deltaTime);
	/// <summary>
	/// スタン姿勢をプロシージャルに作る(スキニングが無いのでクリップではなくコードで)。
	/// 全脚を曲線レイヤーへ移して外へ広げ、胴体を沈め、小さく震わせる。終盤で歩行へ戻す。
	/// </summary>
	void UpdateStunPose(float deltaTime);

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
		KUJATA_REGISTER_STRING_NAMED_TIP(shockwavePrefabPath_, "Shockwave Prefab",
		    "衝撃刃(足攻撃・コマ回転の広い当たり判定)のPrefab。\n"
		    "脚の先端の球だけでは振り回した軌跡を拾えないので、面の判定をこれで出す。空なら衝撃刃なし。");
		KUJATA_REGISTER_INT_NAMED_TIP(bodyPartLayer_, "Body Part Layer", 1.0f, 0, 31,
		    "ボスの体(ルートと全ての子孫)を置くレイヤー番号。\n"
		    "カメラの Obstacle Mask からこのビットを外すと、脚がカメラを押しのけなくなる。\n"
		    "当たり判定は残るので、攻撃も被弾も従来どおり。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(stunSpread_, "Stun Spread", 0.05f, 0.0f, 10.0f,
		    "スタン中に脚を外へ投げ出す距離(ルート中心からの水平距離)。大きいほどへたり込んだ見た目になる。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(stunSink_, "Stun Sink", 0.05f, 0.0f, 5.0f,
		    "スタン中に胴体を沈める量。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(stunTremble_, "Stun Tremble", 0.01f, 0.0f, 1.0f,
		    "スタン中の胴体の震え幅(上下)。0で震えない。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(flinchDuration_, "Flinch Duration", 0.05f, 0.0f, 3.0f,
		    "ジャストガードされたときののけぞり秒数。この間は行動しない。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(criticalRecoilPitch_, "Critical Recoil Pitch", 0.01f, 0.0f, 1.5f,
		    "致命を受けたときに胴体が反り返る角度[rad]。**ここが大きいほど「効いた」感じになる**。0.5前後が目安。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(criticalRecoilSink_, "Critical Recoil Sink", 0.01f, 0.0f, 3.0f,
		    "致命を受けたときに胴体が持ち上がる量。反り返りと合わせて後ろへ倒れかける形になる。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(flinchSink_, "Flinch Sink", 0.05f, 0.0f, 3.0f,
		    "のけぞり時に胴体を沈める量(一瞬沈んで戻る)。");
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
	// 衝撃刃のPrefab。
	KUJATA_FIELD_STRING(shockwavePrefabPath_, "Prefabs/GuardianShockwave.prefab.json");
	// ボスの体を置くレイヤー(カメラの障害物判定から外すため)。
	KUJATA_FIELD_INT(bodyPartLayer_, 8);
	// スタン姿勢: 脚の広げ幅。
	KUJATA_FIELD_FLOAT(stunSpread_, 3.4f);
	// スタン姿勢: 胴体の沈み。
	KUJATA_FIELD_FLOAT(stunSink_, 0.9f);
	// スタン姿勢: 震え幅。
	KUJATA_FIELD_FLOAT(stunTremble_, 0.05f);
	// のけぞり秒数。
	KUJATA_FIELD_FLOAT(flinchDuration_, 0.4f);
	// のけぞり時の沈み。
	KUJATA_FIELD_FLOAT(flinchSink_, 0.35f);
	// 致命を受けたときの仰け反り。
	KUJATA_FIELD_FLOAT(criticalRecoilPitch_, 0.55f);
	KUJATA_FIELD_FLOAT(criticalRecoilSink_, 0.9f);

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

	// --- 衝撃刃(広い当たり判定)の実行状態 ---
	// Prefabから生成した器(1体につき1つ。Playインスタンス内なので停止で消える)。
	KujataEngine::GameObject* shockwave_ = nullptr;
	// 生成を試みたか(失敗時に毎フレーム再試行しないため)。
	bool shockwaveTried_ = false;
	// 出ている残り時間[s]。0以下かつ shockwaveHold_ が false なら消える。
	float shockwaveTimer_ = 0.0f;
	// 全体の長さ[s](広がりの補間に使う)。
	float shockwaveDuration_ = 0.0f;
	// 開始半径 / 終了半径。
	float shockwaveStartRadius_ = 0.0f;
	float shockwaveEndRadius_ = 0.0f;
	// 中心(followがfalseのときの固定位置)。
	KujataEngine::Vector3 shockwaveCenter_ = {0.0f, 0.0f, 0.0f};
	// ボスに追従させるか。
	bool shockwaveFollow_ = false;
	// 時間で消えず、StopShockwaveされるまで出しっぱなしにするか。
	bool shockwaveHold_ = false;
	// Prefabのマテリアルが持つ既定色。減衰はこれを基準にαだけ落とす。
	KujataEngine::Vector4 shockwaveBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// --- スタン/のけぞりの実行状態 ---
	// 自分のHP/体勢崩し(OnPlayStartで解決)。
	class EnemyHealth* health_ = nullptr;
	// スタン姿勢の経過時間[s]。
	float stunElapsed_ = 0.0f;
	// のけぞりの残り時間[s]。0より大きい間はBTを回さない。
	float flinchTimer_ = 0.0f;
	// 致命の仰け反りの残り時間と総尺。
	float criticalRecoilTimer_ = 0.0f;
	float criticalRecoilDuration_ = 1.0f;
	// スタン開始時の各脚の足先(ルートローカル)。ここから広げ位置へ補間する。
	std::vector<KujataEngine::Vector3> stunStartLocal_;
};
