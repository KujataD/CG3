#pragma once

#include "../../externals/imgui/imgui.h"
#include <cstdint>
#include <filesystem>

namespace KujakuEngine {

// Gameウィンドウ（RenderTarget表示 + Transformギズモ + オブジェクト選択）を描画する。
class SceneViewWindow {
public:
	// projectRoot はギズモアイコンの読み込み元（ImGuiManagerが保持するProjectRootを渡す）。
	void Draw(const std::filesystem::path& projectRoot, bool* pOpen = nullptr);

private:
	enum class TransformGizmoOperation {
		Translate,
		Rotate,
		Scale,
	};

	void LoadGizmoIcons(const std::filesystem::path& projectRoot);
	void DrawGizmoToolbar(const std::filesystem::path& projectRoot);
	bool DrawGizmoModeButton(const char* id, const char* fallbackLabel, uint32_t textureIndex, TransformGizmoOperation operation, const char* tooltip);
	void DrawTransformGizmo(const ImVec2& imagePosition, const ImVec2& imageSize);
	void HandleGameWindowObjectSelection(const ImVec2& imagePosition, const ImVec2& imageSize);

	TransformGizmoOperation gizmoOperation_ = TransformGizmoOperation::Translate;
	bool transformGizmoUsing_ = false;
	bool gizmoIconsLoaded_ = false;
	uint32_t gizmoTranslateIconIndex_ = 0;
	uint32_t gizmoRotateIconIndex_ = 0;
	uint32_t gizmoScaleIconIndex_ = 0;
};

} // namespace KujakuEngine
