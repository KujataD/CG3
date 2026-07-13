#pragma once

namespace KujakuEngine {

// Gameウィンドウ。メインカメラで描いたGame用RenderTextureを表示するだけの軽量ウィンドウ。
// Sceneウィンドウと違いギズモ/選択/オーバーレイは無い(プレイヤーが見る画面)。
// RenderTextureは固定解像度(1280x720)なのでアスペクト比を保ってレターボックス表示する。
class GameViewWindow {
public:
	void Draw();
};

} // namespace KujakuEngine
