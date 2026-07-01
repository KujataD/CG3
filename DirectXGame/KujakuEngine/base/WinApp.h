#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace KujakuEngine {

/// <summary>
/// ウィンドウ管理クラス
/// </summary>
class WinApp {
public: // 静的メンバ変数
	// クライアント領域のサイズ
	static const int32_t kWindowWidth = 1280;
	static const int32_t kWindowHeight = 720;

	// ウィンドウクラス名
	static const wchar_t kWindowClassName[];

public: // 静的メンバ関数
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static WinApp* GetInstance();

	/// <summary>
	/// ウィンドウプロシージャ
	/// </summary>
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ関数
	/// <param name="title">ウィンドウタイトル</param>
	/// <param name="clientWidth">クライアント領域の幅</param>
	/// <param name="clientHeight">クライアント領域の高さ</param>
	/// <summary>
	/// GameWindowオブジェクトを作成します。
	/// </summary>
	void CreateGameWindow(const std::wstring& title = L"KujakuEngine", int32_t clientWidth = kWindowWidth, int32_t clientHeight = kWindowHeight);

	/// <summary>
	/// ゲームウィンドウの破棄
	/// </summary>
	void TerminateGameWindow();

	/// <returns>trueで終了リクエスト</returns>
	/// <summary>
	/// ProcessMessageを実行します。
	/// </summary>
	bool ProcessMessage();

	// ゲッター
	/// <summary>
	/// Hwndを取得します。
	/// </summary>
	HWND GetHwnd() const { return hwnd_; }
	/// <summary>
	/// HInstanceを取得します。
	/// </summary>
	HINSTANCE GetHInstance() const { return wndClass_.hInstance; }

private:
	/// <summary>
	/// WinAppを実行します。
	/// </summary>
	WinApp() = default;
	/// <summary>
	/// WinAppを実行します。
	/// </summary>
	~WinApp() = default;
	/// <summary>
	/// WinAppを実行します。
	/// </summary>
	WinApp(const WinApp&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	const WinApp& operator=(const WinApp&) = delete;

private: // メンバ変数
	HWND hwnd_ = nullptr;
	WNDCLASSEX wndClass_ = {};
};

} // namespace KujakuEngine