#include "PerformanceWindow.h"

#include "../../externals/imgui/imgui.h"
#include "../base/DirectXCommon.h"
#include "../runtime/PlayState.h"

namespace KujakuEngine {

void PerformanceWindow::Draw(bool* pOpen) {
#ifdef USE_IMGUI
	ImGui::Begin("Performance", pOpen);

	// フレーム時間はImGuiが計測しているものを使う(エディタの実フレームレートを正確に反映)。
	ImGuiIO& io = ImGui::GetIO();
	const float frameMs = io.DeltaTime * 1000.0f;
	const float fps = io.Framerate; // ImGui側で平滑化済み

	frameTimeHistoryMs_[historyOffset_] = frameMs;
	historyOffset_ = (historyOffset_ + 1) % kHistoryCount;

	ImGui::Text("FPS  : %.1f", fps);
	ImGui::Text("Frame: %.3f ms", 1000.0f / (fps > 0.0001f ? fps : 1.0f));

	// 直近のフレーム時間の履歴グラフ(0〜33.3ms=30fpsラインまで)。
	ImGui::PlotLines("##FrameTimeMs", frameTimeHistoryMs_, kHistoryCount, historyOffset_, "Frame ms", 0.0f, 33.3f, ImVec2(0.0f, 60.0f));

	ImGui::Separator();

	// 2画面のどちらが実際に描画されているか(Phase6aの非表示スキップが効いているか確認できる)。
	const bool sceneVisible = IsSceneViewVisible();
	const bool gameVisible = IsGameViewVisible();
	ImGui::Text("Scene view: %s", sceneVisible ? "rendering" : "skipped");
	ImGui::Text("Game view : %s", gameVisible ? "rendering" : "skipped");

	int activeViews = (sceneVisible ? 1 : 0) + (gameVisible ? 1 : 0);
	ImGui::Text("Active render passes: %d", activeViews);

	ImGui::Separator();

	// 各RenderTextureの解像度(Sceneは表示サイズ追従、Gameは固定)。
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ImGui::Text("Scene RT: %d x %d", dxCommon->GetSceneRenderWidth(), dxCommon->GetSceneRenderHeight());
	ImGui::Text("Game RT : %d x %d", dxCommon->GetGameRenderWidth(), dxCommon->GetGameRenderHeight());

	ImGui::End();
#else
	(void)pOpen;
#endif // USE_IMGUI
}

} // namespace KujakuEngine
