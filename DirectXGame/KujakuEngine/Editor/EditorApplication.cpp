#include "EditorApplication.h"
#include "../2d/ImGuiManager.h"
#include "../components/BuiltinComponents.h"
#include "EditorProjectPath.h"
#include "EditorSelection.h"
#include "SceneJsonImporter.h"
#include "../base/DirectXCommon.h"

namespace KujakuEngine {

EditorApplication* EditorApplication::GetInstance() {
	static EditorApplication instance;
	return &instance;
}

void EditorApplication::Initialize() {
	RegisterBuiltinComponents();

	editorMode_ = EditorMode::Edit;
	AddConsoleLog("Editor Mode: Edit");
}

void EditorApplication::BeginFrame() {
#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->Begin();
#endif // USE_IMGUI
}

void EditorApplication::Update() {
#ifdef USE_IMGUI
	// Editor UIはEditorApplicationを入口として更新する。
	ImGuiManager::GetInstance()->DrawEditor();
#endif // USE_IMGUI

	if (ShouldUpdateGame() && currentScene_) {
		currentScene_->Update();
	}
}

void EditorApplication::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	dxCommon->PreDraw();

#ifdef USE_IMGUI
	// Gameウィンドウ用RenderTargetへゲーム描画を流す。
	dxCommon->BeginGameRender();
#endif // USE_IMGUI

	if (currentScene_) {
		currentScene_->Draw();
	}

#ifdef USE_IMGUI
	// Gameウィンドウ用RenderTargetをImGuiから読める状態に戻す。
	dxCommon->EndGameRender();
#endif // USE_IMGUI
}

void EditorApplication::EndFrame() {
#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->End();
#endif // USE_IMGUI

	DirectXCommon::GetInstance()->PostDraw();
}

void EditorApplication::Finalize() {
	EditorSelection::GetInstance()->Clear();

	if (currentScene_) {
		currentScene_->Finalize();
		currentScene_.reset();
	}
	editorMode_ = EditorMode::Edit;
}

void EditorApplication::Start() {
	if (editorMode_ == EditorMode::Play) {
		return;
	}

	if (currentScene_) {
		currentScene_->OnPlayStart();
	}

	editorMode_ = EditorMode::Play;

#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->ClearConsoleLogs();
#endif // USE_IMGUI
	AddConsoleLog("[Editor] Start");
	AddConsoleLog("Editor Mode: Play");
}

void EditorApplication::Stop() {
	if (editorMode_ == EditorMode::Edit) {
		return;
	}

	editorMode_ = EditorMode::Edit;

	if (currentScene_) {
		currentScene_->OnPlayStop();
	}

	AddConsoleLog("[Editor] Stop");
	AddConsoleLog("Editor Mode: Edit");
}

bool EditorApplication::IsPlaying() const {
	return editorMode_ == EditorMode::Play;
}

EditorMode EditorApplication::GetEditorMode() const {
	return editorMode_;
}

bool EditorApplication::ShouldUpdateGame() const {
	return IsPlaying();
}

void EditorApplication::SetCurrentScene(std::unique_ptr<Scene> scene) {
	EditorSelection::GetInstance()->Clear();

	if (currentScene_) {
		currentScene_->Finalize();
	}

	currentScene_ = std::move(scene);
	if (currentScene_) {
		currentScene_->Initialize();
		SceneJsonImporter::ImportResult importResult = SceneJsonImporter::ImportScene(*currentScene_, DetectEditorProjectRoot());
		if (importResult.imported) {
			if (importResult.succeeded) {
				AddConsoleLog("[Editor] Scene JSON imported: " + importResult.sourceDirectory.string());
			} else {
				AddConsoleLog("[Editor] Scene JSON import failed: " + importResult.message);
			}
		}
	}
}

Scene* EditorApplication::GetCurrentScene() const {
	return currentScene_.get();
}

void EditorApplication::AddConsoleLog(const std::string& message) {
#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->AddConsoleLog(message);
#else
	(void)message;
#endif // USE_IMGUI
}

} // namespace KujakuEngine
