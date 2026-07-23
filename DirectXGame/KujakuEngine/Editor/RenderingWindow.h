#pragma once

namespace KujakuEngine {

// レンダリング設定ウィンドウ。ブルーム/露出/トーンマップ/簡易グレーディング/ビネットを調整する。
// 変更は即時反映され、ProjectSettings/RenderSettings.jsonへ自動保存される。
class RenderingWindow {
public:
	void Draw(bool* pOpen = nullptr);
};

} // namespace KujakuEngine
