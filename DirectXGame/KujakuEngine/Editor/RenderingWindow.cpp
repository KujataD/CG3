#include "RenderingWindow.h"

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#include "../postprocess/PostProcess.h"
#endif // USE_IMGUI

namespace KujakuEngine {

void RenderingWindow::Draw(bool* pOpen) {
#ifdef USE_IMGUI
	if (!ImGui::Begin("Rendering", pOpen)) {
		ImGui::End();
		return;
	}

	PostProcess* postProcess = PostProcess::GetInstance();
	PostProcessSettings& settings = postProcess->GetSettings();
	bool changed = false;

	// 閾値/Soft KneeはMaterialのEmission項目(マテリアル別)へ移行した。ここは全体設定のみ。
	ImGui::SeparatorText("Bloom");
	changed |= ImGui::Checkbox("Enabled", &settings.bloomEnabled);
	changed |= ImGui::DragFloat("Intensity", &settings.bloomIntensity, 0.01f, 0.0f, 5.0f);

	ImGui::SeparatorText("Exposure / Tonemap");
	changed |= ImGui::DragFloat("Exposure", &settings.exposure, 0.01f, 0.0f, 10.0f);
	// 0=None(HDR化の検証用) / 1=Reinhard / 2=ACES。通常はACES。
	const char* tonemapItems[] = {"None", "Reinhard", "ACES"};
	int tonemapIndex = settings.tonemapper;
	if (ImGui::Combo("Tonemapper", &tonemapIndex, tonemapItems, IM_ARRAYSIZE(tonemapItems))) {
		settings.tonemapper = tonemapIndex;
		changed = true;
	}

	ImGui::SeparatorText("Color Grading");
	changed |= ImGui::SliderFloat("Saturation", &settings.saturation, 0.0f, 2.0f);
	changed |= ImGui::SliderFloat("Contrast", &settings.contrast, 0.0f, 2.0f);
	changed |= ImGui::ColorEdit3("Color Filter", &settings.colorFilter.x);

	ImGui::SeparatorText("Vignette");
	changed |= ImGui::SliderFloat("Vignette Intensity", &settings.vignetteIntensity, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Vignette Smoothness", &settings.vignetteSmoothness, 0.01f, 1.0f);

	// フェードは演出用のランタイム状態なので保存対象外。動作確認用に現在値だけ表示する。
	if (postProcess->GetFadeAmount() > 0.0f) {
		ImGui::Separator();
		ImGui::Text("Fade: %.2f", postProcess->GetFadeAmount());
	}

	if (changed) {
		postProcess->SaveSettings();
	}

	ImGui::End();
#else
	(void)pOpen;
#endif // USE_IMGUI
}

} // namespace KujakuEngine
