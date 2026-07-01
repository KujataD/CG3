#pragma once
#include "../math/Vector2.h"
#include <Windows.h>
#include <cassert>

// DirectInputのバージョン指定
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Xinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")

namespace KujakuEngine {

/// <summary>
/// Inputクラスを表します。
/// </summary>
class Input {
public:
	/// <summary>
	/// InputDeviceTypeの種類を表します。
	/// </summary>
	enum class InputDeviceType {
		kKeyboardMouse,
		kController,
	};

	/// <summary>
	/// 初期化します。
	/// </summary>
	static void Initialize();

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	static void Update();

	/// <summary>
	/// Keyを取得します。
	/// </summary>
	static bool GetKey(unsigned char keycode) { return key_[keycode]; }
	/// <summary>
	/// PreKeyを取得します。
	/// </summary>
	static bool GetPreKey(unsigned char keycode) { return preKey_[keycode]; }
	/// <summary>
	/// KeyTriggerを取得します。
	/// </summary>
	static bool GetKeyTrigger(unsigned char keycode) { return key_[keycode] && !preKey_[keycode]; }
	/// <summary>
	/// KeyReleaseを取得します。
	/// </summary>
	static bool GetKeyRelease(unsigned char keycode) { return !key_[keycode] && preKey_[keycode]; }

	/// <summary>
	/// Clickを取得します。
	/// </summary>
	static bool GetClick(int num) { return mouseState_.rgbButtons[num]; }
	/// <summary>
	/// PreClickを取得します。
	/// </summary>
	static bool GetPreClick(int num) { return preMouseState_.rgbButtons[num]; }
	/// <summary>
	/// ClickTriggerを取得します。
	/// </summary>
	static bool GetClickTrigger(int num) { return mouseState_.rgbButtons[num] && !preMouseState_.rgbButtons[num]; }
	/// <summary>
	/// ClickReleaseを取得します。
	/// </summary>
	static bool GetClickRelease(int num) { return !mouseState_.rgbButtons[num] && preMouseState_.rgbButtons[num]; }

	/// <summary>
	/// CurrentInputDeviceTypeを取得します。
	/// </summary>
	static InputDeviceType GetCurrentInputDeviceType() { return currentInputDeviceType_; }
	/// <summary>
	/// KeyboardMouseInputかどうかを返します。
	/// </summary>
	static bool IsKeyboardMouseInput() { return currentInputDeviceType_ == InputDeviceType::kKeyboardMouse; }
	/// <summary>
	/// ControllerInputかどうかを返します。
	/// </summary>
	static bool IsControllerInput() { return currentInputDeviceType_ == InputDeviceType::kController; }

	/// <returns></returns>
	/// <summary>
	/// MouseClientPosを取得します。
	/// </summary>
	static Vector2 GetMouseClientPos();

	/// <returns></returns>
	/// <summary>
	/// MousePreClientPosを取得します。
	/// </summary>
	static Vector2 GetMousePreClientPos();

	// コントローラーが接続されているかを取得する。
	/// <summary>
	/// ControllerConnectedかどうかを返します。
	/// </summary>
	static bool IsControllerConnected(int padNo = 0);

	// コントローラーのボタン入力を取得する。
	/// <summary>
	/// ControllerButtonを取得します。
	/// </summary>
	static bool GetControllerButton(WORD button, int padNo = 0);
	/// <summary>
	/// PreControllerButtonを取得します。
	/// </summary>
	static bool GetPreControllerButton(WORD button, int padNo = 0);
	/// <summary>
	/// ControllerButtonTriggerを取得します。
	/// </summary>
	static bool GetControllerButtonTrigger(WORD button, int padNo = 0);
	/// <summary>
	/// ControllerButtonReleaseを取得します。
	/// </summary>
	static bool GetControllerButtonRelease(WORD button, int padNo = 0);

	// 左右スティックの入力を取得する。
	/// <summary>
	/// LeftStickを取得します。
	/// </summary>
	static Vector2 GetLeftStick(int padNo = 0);
	/// <summary>
	/// RightStickを取得します。
	/// </summary>
	static Vector2 GetRightStick(int padNo = 0);

	// 左右トリガーの入力を取得する。
	/// <summary>
	/// LeftTriggerを取得します。
	/// </summary>
	static float GetLeftTrigger(int padNo = 0);
	/// <summary>
	/// RightTriggerを取得します。
	/// </summary>
	static float GetRightTrigger(int padNo = 0);

private:

	/// <returns></returns>
	/// <summary>
	/// MouseClientPosを計算します。
	/// </summary>
	static Vector2 CalcMouseClientPos();
	/// <summary>
	/// InputDeviceType更新処理を行います。
	/// </summary>
	static void UpdateInputDeviceType();
	/// <summary>
	/// KeyboardMouseInputDetectedかどうかを返します。
	/// </summary>
	static bool IsKeyboardMouseInputDetected();
	/// <summary>
	/// ControllerInputDetectedかどうかを返します。
	/// </summary>
	static bool IsControllerInputDetected(int padNo);
private:
	static HWND hwnd_;

	static IDirectInput8* directInput_;
	static IDirectInputDevice8* keyboard_;
	static IDirectInputDevice8* mouse_;

	// キーボード入力状態
	static BYTE key_[256];
	static BYTE preKey_[256];

	// マウス入力状態
	static DIMOUSESTATE2 mouseState_;
	static DIMOUSESTATE2 preMouseState_;

	// 最大4台分のコントローラー入力状態を保持する。
	static XINPUT_STATE controllerState_[XUSER_MAX_COUNT];
	static XINPUT_STATE preControllerState_[XUSER_MAX_COUNT];
	static bool isControllerConnected_[XUSER_MAX_COUNT];

	static Vector2 mouseClientPos_;
	static Vector2 mousePreClientPos_;
	static InputDeviceType currentInputDeviceType_;
};

}; // namespace KujakuEngine
