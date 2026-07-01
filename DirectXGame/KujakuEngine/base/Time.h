#pragma once
#include <windows.h>

// 参考資料:
// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
// https://learn.microsoft.com/en-us/windows/win32/dxtecharts/game-timing-and-multicore-processors


/// <summary>
/// Timeクラスを表します。
/// </summary>
class Time {
public:
	
	/// <summary>
	/// Instanceを取得します。
	/// </summary>
	static Time* GetInstance();

	/// <summary>
	/// DeltaTimeを取得します。
	/// </summary>
	static float GetDeltaTime() { return GetInstance()->DeltaTime(); }

	/// <summary>
	/// Initを実行します。
	/// </summary>
	void Init();

	/// <summary>
	/// 更新処理を行います。
	/// </summary>
	void Update();

	/// <summary>
	/// DeltaTimeを実行します。
	/// </summary>
	float DeltaTime() const { return dt_; }

private:
	/// <summary>
	/// Timeを実行します。
	/// </summary>
	Time() = default;
	/// <summary>
	/// Timeを実行します。
	/// </summary>
	~Time() = default;
	/// <summary>
	/// Timeを実行します。
	/// </summary>
	Time(const Time&) = delete;
	/// <summary>
	/// operator=を実行します。
	/// </summary>
	const Time& operator=(const Time&) = delete;

private:
	float dt_;

	float maxDeltaTime_;

	// 周波数取得
	LARGE_INTEGER frequency_;

	// 開始時刻を取得
	LARGE_INTEGER prevTime_;

};