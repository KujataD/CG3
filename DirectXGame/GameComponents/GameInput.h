#pragma once
#include <KujataEngine.h>

/// <summary>
/// プレイヤー操作のボタン割り当てを一か所にまとめる(Player / LockOn / PartyManager が共有する)。
/// パッドはXInput、キーボードはDirectInputのキーコード。変えたいときはここだけ触る。
///
///   移動      : 左スティック / WASD
///   カメラ    : 右スティック / 矢印キー(OrbitCameraComponent側)
///   攻撃      : R2(短押し=通常, 長押し=溜め) / K
///   回避      : A / Space
///   ガード    : L2(押している間) / J
///   Z注目     : R3(トグル) / Q。注目中に右スティックを左右へ倒した瞬間で対象切替
///   キャラ切替: 十字キー下 / Tab
/// </summary>
namespace GameInput {

// --- パッド ---
inline constexpr WORD kDodgeButton = XINPUT_GAMEPAD_A;
inline constexpr WORD kLockOnButton = XINPUT_GAMEPAD_RIGHT_THUMB;
inline constexpr WORD kSwapCharacterButton = XINPUT_GAMEPAD_DPAD_DOWN;
inline constexpr WORD kCriticalButton = XINPUT_GAMEPAD_B;
// トリガーは0〜1のアナログ値。この値以上で「押した」とみなす。
inline constexpr float kTriggerThreshold = 0.5f;

// --- キーボード ---
inline constexpr unsigned char kAttackKey = DIK_K;
inline constexpr unsigned char kDodgeKey = DIK_SPACE;
inline constexpr unsigned char kGuardKey = DIK_J;
inline constexpr unsigned char kLockOnKey = DIK_Q;
inline constexpr unsigned char kSwapCharacterKey = DIK_TAB;
// 致命の一撃(スタンした敵に近づくと出るプロンプト)。攻撃・ガードと別のボタンにして誤爆を防ぐ。
inline constexpr unsigned char kCriticalKey = DIK_E;

/// <summary>攻撃ボタン(R2/K)が押されているか。</summary>
inline bool IsAttackHeld() {
	return KujataEngine::Input::GetRightTrigger() >= kTriggerThreshold || KujataEngine::Input::GetKey(kAttackKey);
}

/// <summary>ガードボタン(L2/J)が押されているか。</summary>
inline bool IsGuardHeld() {
	return KujataEngine::Input::GetLeftTrigger() >= kTriggerThreshold || KujataEngine::Input::GetKey(kGuardKey);
}

/// <summary>回避が押された瞬間か。</summary>
inline bool IsDodgeTriggered() {
	return KujataEngine::Input::GetControllerButtonTrigger(kDodgeButton) || KujataEngine::Input::GetKeyTrigger(kDodgeKey);
}

/// <summary>Z注目が押された瞬間か。</summary>
inline bool IsLockOnTriggered() {
	return KujataEngine::Input::GetControllerButtonTrigger(kLockOnButton) || KujataEngine::Input::GetKeyTrigger(kLockOnKey);
}

/// <summary>致命の一撃が押された瞬間か。</summary>
inline bool IsCriticalTriggered() {
	return KujataEngine::Input::GetControllerButtonTrigger(kCriticalButton) || KujataEngine::Input::GetKeyTrigger(kCriticalKey);
}

/// <summary>キャラ切替が押された瞬間か。</summary>
inline bool IsSwapCharacterTriggered() {
	return KujataEngine::Input::GetControllerButtonTrigger(kSwapCharacterButton) || KujataEngine::Input::GetKeyTrigger(kSwapCharacterKey);
}

/// <summary>移動入力(左スティック+WASD合成、カメラ基準のx=横/z=前後)。</summary>
inline KujataEngine::Vector3 GetMoveInput() {
	KujataEngine::Vector2 stick = KujataEngine::Input::GetLeftStick();
	KujataEngine::Vector3 input = {stick.x, 0.0f, stick.y};
	if (KujataEngine::Input::GetKey(DIK_W)) {
		input.z += 1.0f;
	}
	if (KujataEngine::Input::GetKey(DIK_S)) {
		input.z -= 1.0f;
	}
	if (KujataEngine::Input::GetKey(DIK_D)) {
		input.x += 1.0f;
	}
	if (KujataEngine::Input::GetKey(DIK_A)) {
		input.x -= 1.0f;
	}
	return input;
}

} // namespace GameInput
