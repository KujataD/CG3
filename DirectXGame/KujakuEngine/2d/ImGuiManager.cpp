#include "ImGuiManager.h"
#include "../../externals/imgui/imgui_internal.h"
#include "../../externals/ImGuizmo-1.9/src/ImGuizmo.h"
#include "../Editor/EditorApplication.h"
#include "../Editor/EditorSelection.h"
#include "../Editor/SceneJsonExporter.h"
#include "../3d/Camera.h"
#include "../components/TransformComponent.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "../scene/ComponentFactory.h"
#include "../math/MathUtil.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace KujakuEngine {
namespace {

constexpr float kDegreesToRadians = 0.017453292519943295f;

void AllocateImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	// ImGui 1.92系のDX12バックエンドは、フォントや追加テクスチャ用のSRVをバックエンド側へ要求してくる。
	// KujakuEngine側のSRV採番関数を使うことで、Gameウィンドウ用SRVや通常テクスチャと番号が衝突しないようにする。
	DirectXCommon* dxCommon = static_cast<DirectXCommon*>(info->UserData);
	uint32_t srvIndex = dxCommon->AllocateSrvIndex();
	ID3D12DescriptorHeap* srvHeap = dxCommon->GetSrvDescriptorHeap();
	uint32_t descriptorSize = dxCommon->GetDescriptorSizeSRV();

	// CPUハンドルはCreateShaderResourceViewなど、CPU側からDescriptorを書き込む用途で使う。
	*outCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	outCpuHandle->ptr += descriptorSize * srvIndex;

	// GPUハンドルはシェーダーやImGui描画時にGPU側から参照する用途で使う。
	*outGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	outGpuHandle->ptr += descriptorSize * srvIndex;
}

void FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {
	// 現状はSRVヒープをフレーム中に再利用しないため解放処理は不要。
}

float* MatrixData(Matrix4x4& matrix) {
	return &matrix.m[0][0];
}

const float* MatrixData(const Matrix4x4& matrix) {
	return &matrix.m[0][0];
}

} // namespace

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize() {
#ifdef USE_IMGUI

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	WinApp* winApp = WinApp::GetInstance();

	// ImGui初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Dockingを有効化する。これによりDockSpaceや各ウィンドウのドッキングが使える。
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// キーボード操作を有効化して、エディタUIとして最低限扱いやすくする。
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// 1.92系のDX12バックエンドはInitInfoで初期化する。
	// 旧式の初期化ではフォントアトラス用テクスチャが正しく作られず、assertの原因になる。
	ImGui_ImplDX12_InitInfo initInfo{};
	initInfo.Device = dxCommon->GetDevice();
	initInfo.CommandQueue = dxCommon->GetCommandQueue();
	initInfo.NumFramesInFlight = dxCommon->GetSwapChainBufferCount();
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();
	initInfo.UserData = dxCommon;
	initInfo.SrvDescriptorAllocFn = AllocateImGuiSrvDescriptor;
	initInfo.SrvDescriptorFreeFn = FreeImGuiSrvDescriptor;
	ImGui_ImplDX12_Init(&initInfo);

	// 初期レイアウトはImGui初期化後、最初のDrawDockSpaceで構築する。
	dockLayoutInitialized_ = false;
	consoleLogs_.clear();
	AddConsoleLog("Console initialized.");
#endif // USE_IMGUI
}

void ImGuiManager::AddConsoleLog(const std::string& message) {
	consoleLogs_.push_back(message);
	// 無制限にログを溜めるとメモリと描画負荷が増えるため、簡易Consoleでは古いログから捨てる。
	if (consoleLogs_.size() > 128) {
		consoleLogs_.erase(consoleLogs_.begin());
	}
}

void ImGuiManager::ClearConsoleLogs() {
	consoleLogs_.clear();
}

void ImGuiManager::DrawDockSpace() {
#ifdef USE_IMGUI
	// MainViewport全体を覆う透明な親ウィンドウを作り、その中にDockSpaceを置く。
	// これによりGame/Hierarchy/Inspector/ConsoleをUnity風に分割配置できる。
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	// DockSpace用の親ウィンドウは移動・リサイズ・タイトルバー表示をさせず、画面全体の土台として使う。
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// 親ウィンドウの余白や角丸を消して、アプリ全体がエディタ領域になるようにする。
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("KujakuEditorDockSpace", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	if (ImGui::BeginMenuBar()) {
		// EditはStopと同じ扱い。今はPlayを止めて編集状態に戻すだけにしている。
		if (ImGui::Button("Edit")) {
			EditorApplication::GetInstance()->Stop();
		}
		ImGui::SameLine();
		// Startを押すとPlayへ入り、EditorApplication側でゲームロジックが進む。
		if (ImGui::Button("Start")) {
			EditorApplication::GetInstance()->Start();
		}
		ImGui::SameLine();
		// Stopを押すとEditへ戻り、EditorApplication側でゲームロジックが止まる。
		if (ImGui::Button("Stop")) {
			EditorApplication::GetInstance()->Stop();
		}
		ImGui::SameLine();

		// 現在のモードをメニューバー上にも出して、Start/Stopの結果をすぐ確認できるようにする。
		if (EditorApplication::GetInstance()->IsPlaying()) {
			ImGui::TextUnformatted("Mode: Play");
		} else {
			ImGui::TextUnformatted("Mode: Edit");
		}
		ImGui::EndMenuBar();
	}

	// DockSpaceのIDは毎フレーム同じ値にする必要がある。IDが変わるとドッキング状態を維持できない。
	ImGuiID dockspaceId = ImGui::GetID("KujakuEditorMainDockSpace");
	ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

	// DockBuilderによる初期配置は初回だけ。以降はユーザーがドラッグした配置を尊重する。
	if (!dockLayoutInitialized_) {
		SetupInitialLayout(dockspaceId);
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::SetupInitialLayout(ImGuiID dockspaceId) {
#ifdef USE_IMGUI
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// 既存ノードを一度消してから、Unityの2 by 3 Layoutに近い初期Dock構成を作る。
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
	ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

	// gameNodeを少しずつ分割して、Game / Hierarchy / Project / Inspector / Consoleを作る。
	ImGuiID hierarchyNode = 0;
	ImGuiID projectNode = 0;
	ImGuiID inspectorNode = 0;
	ImGuiID consoleNode = 0;
	ImGuiID gameNode = dockspaceId;

	// 右側にInspector、Project、Hierarchyの順で列を作る。
	// 画面上ではGame | Hierarchy | Project | Inspectorとなり、ProjectがHierarchyとInspectorの間に入る。
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.25f, &inspectorNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.20f, &projectNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.20f, &hierarchyNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Down, 0.28f, &consoleNode, &gameNode);

	// ウィンドウ名とDock先を紐づける。名前は各ImGui::Beginの文字列と一致させる。
	ImGui::DockBuilderDockWindow("Inspector", inspectorNode);
	ImGui::DockBuilderDockWindow("Project", projectNode);
	ImGui::DockBuilderDockWindow("Hierarchy", hierarchyNode);
	ImGui::DockBuilderDockWindow("Console", consoleNode);
	ImGui::DockBuilderDockWindow("Game", gameNode);
	ImGui::DockBuilderFinish(dockspaceId);

	// ここをtrueにして、次フレーム以降は初期配置を作り直さない。
	dockLayoutInitialized_ = true;
#endif // USE_IMGUI
}

void ImGuiManager::LoadGizmoIcons() {
#ifdef USE_IMGUI
	if (gizmoIconsLoaded_) {
		return;
	}

	std::filesystem::path imageDirectory = projectWindow_.GetProjectRoot() / "KujakuEngine" / "resources" / "images";
	TextureManager* textureManager = TextureManager::GetInstance();

	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_translate.png").string(), gizmoTranslateIconIndex_);
	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_rotate.png").string(), gizmoRotateIconIndex_);
	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_scale.png").string(), gizmoScaleIconIndex_);

	gizmoIconsLoaded_ = true;
#endif // USE_IMGUI
}

bool ImGuiManager::DrawGizmoModeButton(const char* id, const char* fallbackLabel, uint32_t textureIndex, TransformGizmoOperation operation, const char* tooltip) {
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

void ImGuiManager::DrawGizmoToolbar() {
#ifdef USE_IMGUI
	LoadGizmoIcons();

	DrawGizmoModeButton("##GizmoTranslate", "T", gizmoTranslateIconIndex_, TransformGizmoOperation::Translate, "Translate");
	ImGui::SameLine();
	DrawGizmoModeButton("##GizmoRotate", "R", gizmoRotateIconIndex_, TransformGizmoOperation::Rotate, "Rotate");
	ImGui::SameLine();
	DrawGizmoModeButton("##GizmoScale", "S", gizmoScaleIconIndex_, TransformGizmoOperation::Scale, "Scale");
#endif // USE_IMGUI
}

void ImGuiManager::DrawTransformGizmo(const ImVec2& imagePosition, const ImVec2& imageSize) {
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

	TransformComponent* transformComponent = selectedObject->GetTransformComponent();
	if (!transformComponent) {
		return;
	}

	WorldTransform& transform = transformComponent->GetTransform();
	Matrix4x4 gizmoMatrix = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(imagePosition.x, imagePosition.y, imageSize.x, imageSize.y);

	ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	if (gizmoOperation_ == TransformGizmoOperation::Rotate) {
		operation = ImGuizmo::ROTATE;
	} else if (gizmoOperation_ == TransformGizmoOperation::Scale) {
		operation = ImGuizmo::SCALE;
	}

	ImGuizmo::SetGizmoSizeClipSpace(0.2f);

	if (!ImGuizmo::Manipulate(MatrixData(camera->matView), MatrixData(camera->matProjection), operation, ImGuizmo::LOCAL, MatrixData(gizmoMatrix))) {
		return;
	}

	float translation[3]{};
	float rotationDegrees[3]{};
	float scale[3]{};
	ImGuizmo::DecomposeMatrixToComponents(MatrixData(gizmoMatrix), translation, rotationDegrees, scale);

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
#else
	(void)imagePosition;
	(void)imageSize;
#endif // USE_IMGUI
}

void ImGuiManager::DrawGameWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Game");
	DrawGizmoToolbar();
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

	// DirectXCommonが作ったGame用RenderTargetのSRVをImGuiへ渡す。
	// 描画本体はEditorApplicationでBeginGameRenderからEndGameRenderの間に行われる。
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	D3D12_GPU_DESCRIPTOR_HANDLE handle = dxCommon->GetGameRenderSrvHandle();

	float gameAspect = 16.0f / 9.0f;
	if (dxCommon->GetGameRenderHeight() > 0) {
		gameAspect = static_cast<float>(dxCommon->GetGameRenderWidth()) / static_cast<float>(dxCommon->GetGameRenderHeight());
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
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawHierarchyWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Hierarchy");

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		EditorSelection::GetInstance()->Clear();
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	GameObject* selectedObject = EditorSelection::GetInstance()->GetSelectedGameObject();
	bool selectedObjectExists = false;

	for (const std::unique_ptr<GameObject>& object : scene->GetGameObjects()) {
		if (!object) {
			continue;
		}

		GameObject* raw = object.get();
		if (raw == selectedObject) {
			selectedObjectExists = true;
		}

		std::string displayName = raw->GetName();
		if (!raw->IsActive()) {
			displayName = "[Inactive] " + displayName;
		}

		ImGui::PushID(raw);
		if (!raw->IsActive()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		}

		bool isSelected = selectedObject == raw;
		if (ImGui::Selectable(displayName.c_str(), isSelected)) {
			EditorSelection::GetInstance()->SetSelectedGameObject(raw);
		}

		if (!raw->IsActive()) {
			ImGui::PopStyleColor();
		}
		ImGui::PopID();
	}

	if (ImGui::BeginPopupContextWindow("HierarchyCreateMenu", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("Entity")) {
			GameObject* created = scene->CreateEditorEntity();
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}

		if (ImGui::MenuItem("Cube")) {
			GameObject* created = scene->CreateEditorCube();
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}

		if (ImGui::MenuItem("Sphere")) {
			GameObject* created = scene->CreateEditorSphere();
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}

		ImGui::EndPopup();
	}

	if (selectedObject && !selectedObjectExists) {
		EditorSelection::GetInstance()->Clear();
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawInspectorWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Inspector");

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		EditorSelection::GetInstance()->Clear();
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	GameObject* selected = EditorSelection::GetInstance()->GetSelectedGameObject();
	if (!selected) {
		ImGui::TextDisabled("No GameObject selected.");
		ImGui::End();
		return;
	}

	std::array<char, 128> nameBuffer{};
	std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", selected->GetName().c_str());
	if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
		selected->SetName(nameBuffer.data());
	}

	bool active = selected->IsActive();
	if (ImGui::Checkbox("Active", &active)) {
		selected->SetActive(active);
	}

	ImGui::Separator();
	std::vector<std::unique_ptr<Component>>& components = selected->GetComponents();
	if (components.empty()) {
		ImGui::TextDisabled("No Components.");
	}

	Component* removeTarget = nullptr;
	for (const std::unique_ptr<Component>& component : components) {
		if (!component) {
			continue;
		}

		ImGui::PushID(component.get());
		if (ImGui::CollapsingHeader(component->GetTypeName(), ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = component->IsEnabled();
			if (ImGui::Checkbox("Enabled", &enabled)) {
				component->SetEnabled(enabled);
			}

			component->DrawInspector();

			if (component->CanRemove()) {
				if (ImGui::Button("Remove Component")) {
					removeTarget = component.get();
				}
			} else {
				ImGui::TextDisabled("Required Component");
			}
		}
		ImGui::PopID();
	}

	if (removeTarget) {
		selected->RemoveComponent(removeTarget);
	}

	ImGui::Separator();
	if (ImGui::Button("Add Component")) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup")) {
		const std::vector<std::string>& typeNames = ComponentFactory::GetInstance().GetRegisteredTypeNames();
		if (typeNames.empty()) {
			ImGui::TextDisabled("No registered Components.");
		}

		for (const std::string& typeName : typeNames) {
			if (ImGui::MenuItem(typeName.c_str())) {
				Component* added = selected->AddComponent(ComponentFactory::GetInstance().Create(typeName));
				scene->OnEditorComponentAdded(selected, added);
			}
		}

		ImGui::EndPopup();
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawConsoleWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Console");
	// ログとは別に、現在モードを常に先頭へ表示する。
	if (EditorApplication::GetInstance()->IsPlaying()) {
		ImGui::TextUnformatted("Editor Mode: Play");
	} else {
		ImGui::TextUnformatted("Editor Mode: Edit");
	}
	ImGui::Separator();
	for (const std::string& log : consoleLogs_) {
		ImGui::TextUnformatted(log.c_str());
	}
	// Consoleが末尾までスクロールされている場合は、新しいログへ追従する。
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawProjectWindow() {
#ifdef USE_IMGUI
	// ProjectDir以下のフォルダ/ファイルを閲覧するProject Windowを描画する。
	projectWindow_.Draw();
#endif // USE_IMGUI
}

void ImGuiManager::HandleEditorShortcuts() {
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
		ExportCurrentSceneJson();
	}
#endif // USE_IMGUI
}

void ImGuiManager::ExportCurrentSceneJson() {
	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		AddConsoleLog("[Editor] Scene JSON export failed: No Scene.");
		return;
	}

	SceneJsonExporter::ExportResult exportResult = SceneJsonExporter::ExportScene(*scene, projectWindow_.GetProjectRoot());
	if (exportResult.succeeded) {
		AddConsoleLog("[Editor] Scene JSON exported: " + exportResult.outputDirectory.string());
		projectWindow_.Refresh();
		return;
	}

	AddConsoleLog("[Editor] Scene JSON export failed: " + exportResult.message);
}

void ImGuiManager::DrawEditor() {
#ifdef USE_IMGUI
	// Editor UIを構成する各ウィンドウを毎フレーム描画する。
	// 実際の配置はDockSpace側が管理するため、ここでは各ウィンドウを出すだけでよい。
	HandleEditorShortcuts();
	DrawDockSpace();
	DrawGameWindow();
	DrawHierarchyWindow();
	DrawInspectorWindow();
	// ProjectはDockBuilderでHierarchyとInspectorの間に初期配置される。
	DrawProjectWindow();
	DrawConsoleWindow();
#endif // USE_IMGUI
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI

	// エンジンのフレーム先頭処理に対応
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
#endif // USE_IMGUI
}

void ImGuiManager::End() {
#ifdef USE_IMGUI

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	// Project Windowで要求されたモデルサムネイルを、ImGui本体の描画前にOffscreenへ描く。
	projectWindow_.RenderModelPreviews();
	ImGui::Render();
	// ImGuiで作ったUIをDirectX12のCommandListに描画命令として積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());
#endif // USE_IMGUI
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // USE_IMGUI
}

} // namespace KujakuEngine
