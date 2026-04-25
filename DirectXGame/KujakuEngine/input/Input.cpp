#include "Input.h"
#include "../base/WinApp.h"

namespace KujakuEngine{

IDirectInput8* Input::directInput_ = nullptr;
IDirectInputDevice8* Input::keyboard_ = nullptr;
IDirectInputDevice8* Input::mouse_ = nullptr;

BYTE Input::key_[256] = {};
BYTE Input::preKey_[256] = {};

DIMOUSESTATE2 Input::mouseState_ = {};
DIMOUSESTATE2 Input::preMouseState_ = {};

HWND Input::hwnd_;

void Input::Initialize() {
	
	HRESULT hr;
	WinApp* winApp = WinApp::GetInstance();
	
	hwnd_ = winApp->GetHwnd();

	// DirectInputの初期化
	hr = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
	assert(SUCCEEDED(hr));

	// キーボードデバイスの生成
	hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式のセット
	hr = keyboard_->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(hr));

	// 排他制御レベルのセット
	hr = keyboard_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// マウスデバイスの生成
	hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(hr));

	// 入力データ形式セット
	hr = mouse_->SetDataFormat(&c_dfDIMouse2);

	// 排他制御レベルのセット
	mouse_->SetCooperativeLevel(hwnd_, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
}

void Input::Update() {
	// キーボード情報の取得開始
	keyboard_->Acquire();

	// 全キーの入力状態を取得する
	memcpy(preKey_, key_, 256);
	keyboard_->GetDeviceState(sizeof(key_), key_);

	// マウス情報の取得開始
	mouse_->Acquire();

	// 全マウスの入力状態を取得する
	preMouseState_ = mouseState_;
	mouse_->GetDeviceState(sizeof(mouseState_), &mouseState_);
}

Vector2 Input::GetMousePos() {
	POINT mousePoint;
	// マウスカーソルのスクリーン座標を取得
	GetCursorPos(&mousePoint);

	// スクリーン座標を指定のウィンドウのクライアント領域での座標に変換
	ScreenToClient(hwnd_, &mousePoint);

	return Vector2{static_cast<float>(mousePoint.x), static_cast<float>(mousePoint.y)};
}

}