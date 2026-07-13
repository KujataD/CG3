#include "SceneViewWindow.h"

#include "../../externals/ImGuizmo-1.9/src/ImGuizmo.h"
#include "../3d/Camera.h"
#include "../3d/WorldTransform.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../runtime/PlayState.h"
#include "../math/MathUtil.h"
#include "../math/Vector2.h"
#include "../scene/GameObject.h"
#include "../scene/RayCast.h"
#include "../scene/Scene.h"
#include "EditorApplication.h"
#include "EditorImGuiUtil.h"
#include "EditorSelection.h"
#include <cmath>
#include <d3d12.h>

namespace KujakuEngine {
namespace {

constexpr float kDegreesToRadians = 0.017453292519943295f;

float* MatrixData(Matrix4x4& matrix) { return &matrix.m[0][0]; }

const float* MatrixData(const Matrix4x4& matrix) { return &matrix.m[0][0]; }

} // namespace

void SceneViewWindow::LoadGizmoIcons(const std::filesystem::path& projectRoot) {
#ifdef USE_IMGUI
	if (gizmoIconsLoaded_) {
		return;
	}

	std::filesystem::path imageDirectory = projectRoot / "KujakuEngine" / "resources" / "images";
	TextureManager* textureManager = TextureManager::GetInstance();

	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_translate.png").string(), gizmoTranslateIconIndex_);
	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_rotate.png").string(), gizmoRotateIconIndex_);
	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_scale.png").string(), gizmoScaleIconIndex_);

	gizmoIconsLoaded_ = true;
#else
	(void)projectRoot;
#endif // USE_IMGUI
}

bool SceneViewWindow::DrawGizmoModeButton(const char* id, const char* fallbackLabel, uint32_t textureIndex, TransformGizmoOperation operation, const char* tooltip) {
#ifdef USE_IMGUI
	bool selected = gizmoOperation_ == operation;
	if (selected) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}

	bool clicked = false;
	if (textureIndex != 0) {
		D3D12_GPU_DESCRIPTOR_HANDLE handle = TextureManager::GetInstance()->GetSrvHandle(textureIndex);
		clicked = ImGui::ImageButton(id, static_cast<ImTextureID>(handle.ptr), ImVec2(22.0f, 22.0f));
	} else {
		clicked = ImGui::Button(fallbackLabel, ImVec2(32.0f, 32.0f));
	}

	if (selected) {
		ImGui::PopStyleColor();
	}

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", tooltip);
	}

	if (clicked) {
		gizmoOperation_ = operation;
		return true;
	}
#else
	(void)id;
	(void)fallbackLabel;
	(void)textureIndex;
	(void)operation;
	(void)tooltip;
#endif // USE_IMGUI
	return false;
}

void SceneViewWindow::DrawGizmoToolbar(const std::filesystem::path& projectRoot) {
#ifdef USE_IMGUI
	LoadGizmoIcons(projectRoot);

	DrawGizmoModeButton("##GizmoTranslate", "T", gizmoTranslateIconIndex_, TransformGizmoOperation::Translate, "Translate");
	ImGui::SameLine();
	DrawGizmoModeButton("##GizmoRotate", "R", gizmoRotateIconIndex_, TransformGizmoOperation::Rotate, "Rotate");
	ImGui::SameLine();
	DrawGizmoModeButton("##GizmoScale", "S", gizmoScaleIconIndex_, TransformGizmoOperation::Scale, "Scale");
#else
	(void)projectRoot;
#endif // USE_IMGUI
}

void SceneViewWindow::DrawTransformGizmo(const ImVec2& imagePosition, const ImVec2& imageSize) {
#ifdef USE_IMGUI
	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		return;
	}

	Camera* camera = scene->GetEditorCamera();
	if (!camera) {
		return;
	}

	GameObject* selectedObject = EditorSelection::GetInstance()->GetSelectedGameObject();
	if (!selectedObject || !selectedObject->IsActive()) {
		return;
	}

	if (EditorApplication::GetInstance()->IsPlaying()) {
		return;
	}

	WorldTransform& transform = selectedObject->GetTransform();
	selectedObject->UpdateWorldTransformSelfAndAncestors();
	Matrix4x4 gizmoMatrix = transform.matWorld_;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(imagePosition.x, imagePosition.y, imageSize.x, imageSize.y);

	ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	if (gizmoOperation_ == TransformGizmoOperation::Rotate) {
		operation = ImGuizmo::ROTATE;
	} else if (gizmoOperation_ == TransformGizmoOperation::Scale) {
		operation = ImGuizmo::SCALE;
	}

	// ギズモのサイズ変更
	ImGuizmo::SetGizmoSizeClipSpace(0.2f);

	if (!ImGuizmo::Manipulate(MatrixData(camera->matView), MatrixData(camera->matProjection), operation, ImGuizmo::LOCAL, MatrixData(gizmoMatrix))) {
		if (!ImGuizmo::IsUsing()) {
			transformGizmoUsing_ = false;
		}
		return;
	}

	if (!transformGizmoUsing_) {
		CaptureUndo(*scene, "Transform Gizmo");
		transformGizmoUsing_ = true;
	}

	float translation[3]{};
	float rotationDegrees[3]{};
	float scale[3]{};
	Matrix4x4 localGizmoMatrix = gizmoMatrix;
	if (transform.parent_) {
		localGizmoMatrix = gizmoMatrix * Inverse(transform.parent_->matWorld_);
	}

	ImGuizmo::DecomposeMatrixToComponents(MatrixData(localGizmoMatrix), translation, rotationDegrees, scale);

	transform.translation_ = {translation[0], translation[1], translation[2]};
	transform.rotation_ = {
	    rotationDegrees[0] * kDegreesToRadians,
	    rotationDegrees[1] * kDegreesToRadians,
	    rotationDegrees[2] * kDegreesToRadians,
	};
	transform.scale_ = {scale[0], scale[1], scale[2]};

	if (!std::isfinite(transform.scale_.x) || transform.scale_.x < 0.001f) {
		transform.scale_.x = 0.001f;
	}
	if (!std::isfinite(transform.scale_.y) || transform.scale_.y < 0.001f) {
		transform.scale_.y = 0.001f;
	}
	if (!std::isfinite(transform.scale_.z) || transform.scale_.z < 0.001f) {
		transform.scale_.z = 0.001f;
	}
	if (!ImGuizmo::IsUsing()) {
		transformGizmoUsing_ = false;
	}
#else
	(void)imagePosition;
	(void)imageSize;
#endif // USE_IMGUI
}

void SceneViewWindow::HandleGameWindowObjectSelection(const ImVec2& imagePosition, const ImVec2& imageSize) {
#ifdef USE_IMGUI
	if (EditorApplication::GetInstance()->IsPlaying()) {
		return;
	}
	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
		return;
	}
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		return;
	}
	if (ImGuizmo::IsUsing() || ImGuizmo::IsOver()) {
		return;
	}

	ImVec2 mousePosition = ImGui::GetIO().MousePos;
	if (mousePosition.x < imagePosition.x) {
		return;
	}
	if (mousePosition.y < imagePosition.y) {
		return;
	}
	if (mousePosition.x > imagePosition.x + imageSize.x) {
		return;
	}
	if (mousePosition.y > imagePosition.y + imageSize.y) {
		return;
	}

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		return;
	}

	Camera* camera = scene->GetEditorCamera();
	if (!camera) {
		return;
	}

	Ray ray{};
	Vector2 point = {mousePosition.x, mousePosition.y};
	Vector2 viewportPosition = {imagePosition.x, imagePosition.y};
	Vector2 viewportSize = {imageSize.x, imageSize.y};
	if (!RayCast::CreateRayFromViewportPoint(point, viewportPosition, viewportSize, *camera, ray)) {
		return;
	}

	RayCastHit hit{};
	if (RayCast::CastForEditor(*scene, ray, hit)) {
		if (hit.gameObject) {
			EditorSelection::GetInstance()->SetSelectedGameObject(hit.gameObject);
		}
	} else {
		EditorSelection::GetInstance()->Clear();
	}
#else
	(void)imagePosition;
	(void)imageSize;
#endif // USE_IMGUI
}

void SceneViewWindow::Draw(const std::filesystem::path& projectRoot) {
#ifdef USE_IMGUI
	// Begin の戻り値で可視性(タブ非アクティブ/折り畳み時はfalse)を判定し、非表示ならパスをスキップさせる。
	bool visible = ImGui::Begin("Scene");
	SetSceneViewVisible(visible);

	// Sceneビューがフォーカスされている間だけデバッグカメラを操作できるようにする(プレイ中も含む)。
	// これによりGameビュー(メインカメラ)やゲーム入力と操作が競合しない。
	SetSceneViewFocused(visible && ImGui::IsWindowFocused());

	if (!visible) {
		ImGui::End();
		return;
	}

	DrawGizmoToolbar(projectRoot);
	ImGui::Separator();

	// DockされたGameウィンドウの内側サイズを取得する。タブや枠の分を除いた描画可能領域。
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	// Imageに0以下のサイズを渡さないように最低サイズを保証する。
	if (contentSize.x < 1.0f) {
		contentSize.x = 1.0f;
	}
	if (contentSize.y < 1.0f) {
		contentSize.y = 1.0f;
	}

	// DirectXCommonが作ったScene用RenderTexture(デバッグカメラ描画)のSRVをImGuiへ渡す。
	// Scene RTはGameビューと同じく固定解像度(1280x720)。表示側でアスペクト比を保ってレターボックス表示する。
	// (表示サイズ追従は毎フレームのGPU待ち/作り直しで不安定だったため固定に戻した)
	// 描画本体はEditorApplicationでBeginSceneRenderからEndSceneRenderの間に行われる。
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	D3D12_GPU_DESCRIPTOR_HANDLE handle = dxCommon->GetSceneRenderSrvHandle();

	float gameAspect = 16.0f / 9.0f;
	if (dxCommon->GetSceneRenderHeight() > 0) {
		gameAspect = static_cast<float>(dxCommon->GetSceneRenderWidth()) / static_cast<float>(dxCommon->GetSceneRenderHeight());
	}

	ImVec2 imageSize = contentSize;
	ImVec2 imageOffset = {0.0f, 0.0f};
	float contentAspect = contentSize.x / contentSize.y;
	if (contentAspect > gameAspect) {
		imageSize.y = contentSize.y;
		imageSize.x = imageSize.y * gameAspect;
		imageOffset.x = (contentSize.x - imageSize.x) * 0.5f;
	} else {
		imageSize.x = contentSize.x;
		imageSize.y = imageSize.x / gameAspect;
		imageOffset.y = (contentSize.y - imageSize.y) * 0.5f;
	}

	ImVec2 contentPosition = ImGui::GetCursorScreenPos();
	ImVec2 contentEnd = {contentPosition.x + contentSize.x, contentPosition.y + contentSize.y};
	ImVec2 imagePosition = {contentPosition.x + imageOffset.x, contentPosition.y + imageOffset.y};
	ImVec2 imageEnd = {imagePosition.x + imageSize.x, imagePosition.y + imageSize.y};

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(contentPosition, contentEnd, IM_COL32(0, 0, 0, 255));
	drawList->AddImage(static_cast<ImTextureID>(handle.ptr), imagePosition, imageEnd, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
	ImGui::Dummy(contentSize);
	DrawTransformGizmo(imagePosition, imageSize);
	HandleGameWindowObjectSelection(imagePosition, imageSize);
	ImGui::End();
#else
	(void)projectRoot;
#endif // USE_IMGUI
}

} // namespace KujakuEngine
