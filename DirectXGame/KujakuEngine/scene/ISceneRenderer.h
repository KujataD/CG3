#pragma once

namespace KujakuEngine {

class Camera;

// Scene/Game 2画面描画に対応するSceneが実装する能力インターフェース。
// 呼び出し側(EditorApplication)はdynamic_castで問い合わせる。未対応(古いGameModule等)なら
// dynamic_castがnullになり単一ビュー描画へフォールバックする(Scene基底のvtableは変えないので
// ABIを壊さず、古いGameModuleでもクラッシュしない)。Componentの能力分離(IRaycastTarget等)と同じ方針。
class ISceneRenderer {
public:
	virtual ~ISceneRenderer() = default;

	// 毎フレーム1回の準備(入力反映・カメラ同期・ライト・ワールド行列など)。ビュー描画の前に呼ぶ。
	virtual void PrepareFrame() = 0;

	// 1ビューぶんの描画。cameraでモデルを描き、drawEditorOverlaysでグリッド/アイコン/フラスタム等を足す。
	virtual void RenderView(Camera* camera, bool drawEditorOverlays) = 0;

	// Sceneビュー(デバッグカメラ)/Gameビュー(メインカメラ)のカメラ。無ければnullptr。
	virtual Camera* GetSceneViewCamera() = 0;
	virtual Camera* GetGameViewCamera() = 0;
};

} // namespace KujakuEngine
