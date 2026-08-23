#pragma once

#include <KujataEngine.h>

class GuardianGait;
class IGuardianLegRig;

/// <summary>
/// ガーディアンの球体ボディ。脚とは独立に動く「上物」を担当する。
///
/// 脚が地面に合わせて勝手に動くのに対して、ボディは接地している足の位置から
/// 「どのくらいの高さで、どちらへ傾いて浮かんでいるべきか」を毎フレーム決める。
/// 接合部(Leg*_Hip)はこのボディの子なので、ボディが沈めば脚も自動的に畳まれる。
///
/// 接地していない脚(踏み出し中・攻撃で曲線制御中)は平均から除外する。
/// 攻撃で持ち上げた脚に釣られてボディが浮き上がらないようにするため。
///
/// ボディのYaw回転は歩行と完全に独立している。spinSpeedDeg を入れると、
/// 脚がその場に接地したまま球体だけが回り続ける(ガーディアンらしい挙動)。
/// </summary>
class GuardianBody : public KujataEngine::Component {
public:
	const char* GetTypeName() const override { return "GuardianBody"; }
	bool AllowMultiple() const override { return false; }

	void OnPlayStart() override;
	void Update() override;

	/// <summary>
	/// 胴体のYawへ回転を足します。単位はradian。
	///
	/// Updateがrotation_を毎フレーム丸ごと書き直すので、外から直接Transformを触っても
	/// 次のフレームで消える。演出や攻撃から胴体を回したい場合はここを通すこと。
	/// </summary>
	void AddSpin(float radians) { currentSpin_ += radians; }

	/// <summary>
	/// 胴体の高さへ加算するオフセット[unit]。負で沈み込む。
	/// 溜めやジャンプの予備動作など、一時的に車高を変えたいときに使う。
	/// Ride Height を直接いじると平滑化を通って戻りが鈍いので、こちらは即座に効く。
	/// </summary>
	void SetHeightOffset(float offset) { heightOffset_ = offset; }
	float GetHeightOffset() const { return heightOffset_; }

	/// <summary>
	/// 足から求めた傾きへ足す追加のピッチ[rad]。**致命を受けた大きな仰け反り**のように、
	/// 地形とは無関係に胴体を反らせたいときに使う。0で通常。
	/// </summary>
	void SetPitchOffset(float radian) { pitchOffset_ = radian; }
	float GetPitchOffset() const { return pitchOffset_; }

private:
	/// <summary>接地中の脚だけを集めて、平均の高さと前後左右の高低差を求めます。</summary>
	bool GatherPlantedFeet(
	    IGuardianLegRig& rig, const GuardianGait* gait, float& outAverageHeight, float& outPitchSlope, float& outRollSlope) const;

public:
	KUJATA_SERIALIZED_FIELDS_BEGIN() {
		KUJATA_REGISTER_FLOAT_NAMED_TIP(rideHeight_, "Ride Height", 0.01f, 0.0f, 30.0f,
		    "接地している足の平均高さから、ボディ中心をどれだけ上に置くか。いわば車高。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(maxSuspensionTravel_, "Max Suspension Travel", 0.01f, 0.0f, 30.0f,
		    "足の高さへの追従で、ボディが Ride Height からずれてよい最大量(サスペンションのストローク)。\n"
		    "**これが無いとジャンプできない。** ルートのYが完全に打ち消され、\n"
		    "ボディのワールド高さが「足の高さ + Ride Height」に固定されてしまうため。\n"
		    "ストロークを超える段差には追従しきれないので、地形の起伏より少し大きめに取る。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(heightSmoothing_, "Height Smoothing", 0.05f, 0.0f, 60.0f,
		    "高さ追従の速さ[1/秒]。大きいほど地形に機敏に反応する。0で追従しない。");
		KUJATA_REGISTER_BOOL_NAMED_TIP(alignToFeet_, "Align To Feet",
		    "接地している足の高低差に合わせてボディを傾ける。\n"
		    "踏み出し中や曲線制御中の脚は平均から除外されるので、\n"
		    "攻撃で持ち上げた脚に釣られて傾いたりはしない。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(tiltStrength_, "Tilt Strength", 0.01f, -5.0f, 5.0f,
		    "傾きの強さ。**符号を反転すると傾く向きが逆になる。**\n"
		    "モデルの前後の向きに合わせて調整する。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(maxTiltDeg_, "Max Tilt (deg)", 0.1f, 0.0f, 89.0f,
		    "傾きの上限。急な段差で極端に傾くのを防ぐ。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(tiltSmoothing_, "Tilt Smoothing", 0.05f, 0.0f, 60.0f,
		    "傾き追従の速さ[1/秒]。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(bobAmplitude_, "Bob Amplitude", 0.005f, 0.0f, 5.0f,
		    "歩行中の上下の揺れ幅。移動速度に比例するので、止まれば揺れない。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(bobFrequency_, "Bob Frequency", 0.01f, 0.0f, 20.0f,
		    "揺れの速さ[Hz相当]。歩調とは連動していないので、合わないときはここで合わせる。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(bobSpeedReference_, "Bob Speed Reference", 0.01f, 0.01f, 50.0f,
		    "この移動速度[unit/秒]で揺れ幅が最大になる。これ以上速くても揺れは増えない。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(idleSwayAmplitude_, "Idle Sway Amplitude", 0.005f, 0.0f, 2.0f,
		    "立ち止まっている時に胴体を上下させる量。歩行中のBobとは別物で、\n"
		    "**止まっている間に効く**。0で無効。Bob Amplitudeの半分くらいが目安。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(idleSwayFrequency_, "Idle Sway Frequency", 0.01f, 0.01f, 5.0f,
		    "胴体の揺れの速さ[Hz相当]。脚(Idle Jitter Frequency)とは別の値にしておくと、\n"
		    "周期が噛み合わずに自然な軋みになる。");
		KUJATA_REGISTER_FLOAT_NAMED_TIP(spinSpeedDeg_, "Spin Speed (deg/s)", 0.5f, -720.0f, 720.0f,
		    "ボディだけを回し続ける速度。**脚は接地したまま球体だけが回る。**\n"
		    "足の定位置がルート基準なので、ボディの回転に脚は影響されない。");
	}

private:
	// 接地している足の平均高さから、ボディ中心をどれだけ上に置くか。
	// 低くするほど脚が横へ寝て、踏ん張った姿勢になる。
	float rideHeight_ = 1.7f;

	// 足の高さへの追従で、ボディが rideHeight_ からずれてよい最大量(サスペンションのストローク)。
	// これが無いとルートのYが完全に打ち消され、ルートを上げてもボディのワールド高さが
	// 「足の高さ + rideHeight_」に固定される。結果として接合部が上がらず脚が伸びきらないので、
	// GuardianGait の離脱判定が発火せずジャンプにならない。
	// このストロークを超える段差には追従しきれなくなるので、地形の起伏より少し大きめに取る。
	float maxSuspensionTravel_ = 0.6f;
	// 高さ追従の速さ[1/s]。大きいほど地形に機敏に反応する。0で追従しない。
	float heightSmoothing_ = 8.0f;

	// 足の高低差からボディを傾けるか。
	bool alignToFeet_ = true;
	// 傾きの強さ。符号を反転すると傾く向きが逆になる(モデルの向きに合わせて調整する)。
	float tiltStrength_ = 1.0f;
	// 傾きの上限[deg]。
	float maxTiltDeg_ = 20.0f;
	// 傾き追従の速さ[1/s]。
	float tiltSmoothing_ = 6.0f;

	// 歩行中の上下の揺れ幅。
	float bobAmplitude_ = 0.12f;
	// 揺れの速さ[Hz相当]。
	float bobFrequency_ = 2.4f;
	// この速度[unit/s]で揺れ幅が最大になる。
	float bobSpeedReference_ = 4.0f;

	// --- 待機中の揺れ(パーリンノイズ) ---
	// 歩行中のBobは正弦波だが、こちらはノイズ。周期が読めないので「生きている」感じになる。
	float idleSwayAmplitude_ = 0.05f;
	float idleSwayFrequency_ = 0.22f;

	// ボディだけを回し続ける速度[deg/s]。脚は接地したまま球体が回る。
	float spinSpeedDeg_ = 0.0f;

	// --- 実行状態 ---
	// 平滑後のボディのローカル高さ。
	float currentHeight_ = 0.0f;
	// 平滑後の傾き[rad]。
	float currentPitch_ = 0.0f;
	float currentRoll_ = 0.0f;
	// ボディの独立Yaw[rad]。
	float currentSpin_ = 0.0f;
	// ボビングの位相[rad]。
	float bobPhase_ = 0.0f;
	// ノイズに入れる時刻[s]。
	float noiseTime_ = 0.0f;
	// 外部から与える高さオフセット(平滑化を通さず即座に効く)。
	float heightOffset_ = 0.0f;
	// 外部から与える追加ピッチ[rad](致命の仰け反りなど)。平滑化を通さず即座に効く。
	float pitchOffset_ = 0.0f;
	// 初回フレームは平滑せず即座に合わせる。
	bool initialized_ = false;
};
