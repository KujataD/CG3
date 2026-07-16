#pragma once

#include "KujakuApi.h"
#include <functional>
#include <string>

namespace KujakuEngine {

/// <summary>
/// UIの名前付きイベントバス。ButtonのonClick等をエンジン/ゲーム境界をまたいで配信する。
/// ゲーム側はSubscribe(eventName, callback)で購読し、UI側はPublish(eventName)で発火する。
/// </summary>
class UIEventBus {
public:
	static KUJAKU_API void Subscribe(const std::string& eventName, std::function<void()> callback);
	static KUJAKU_API void Publish(const std::string& eventName);

	/// <summary>Scene切り替え等で購読を全消去する。</summary>
	static KUJAKU_API void Clear();
};

} // namespace KujakuEngine
