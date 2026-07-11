#pragma once
#include <windows.h>

#include "../runtime/KujakuApi.h"

// 参考資料:
// https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
// https://learn.microsoft.com/en-us/windows/win32/dxtecharts/game-timing-and-multicore-processors

class KUJAKU_API Time {
public:
	
	static Time* GetInstance();
	static float GetDeltaTime() { return GetInstance()->DeltaTime(); }

	void Init();
	void Update();
	float DeltaTime() const { return dt_; }

private:
	Time() = default;
	~Time() = default;
	Time(const Time&) = delete;
	const Time& operator=(const Time&) = delete;

private:
	float dt_;

	float maxDeltaTime_;

	// 周波数取得
	LARGE_INTEGER frequency_;

	// 開始時刻を取得
	LARGE_INTEGER prevTime_;

};