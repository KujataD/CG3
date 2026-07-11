#include "ImGuiManager.h"
#include "../../externals/ImGuizmo-1.9/src/ImGuizmo.h"
#include "../../externals/imgui/imgui_internal.h"
#include "../3d/Camera.h"
#include "../assets/MaterialAsset.h"
#include "../Editor/AssetDatabase.h"
#include "../Editor/EditorApplication.h"
#include "../Editor/EditorConsole.h"
#include "../Editor/EditorImGuiUtil.h"
#include "../Editor/EditorProjectPath.h"
#include "../Editor/EditorSelection.h"
#include "../Editor/EditorStyle.h"
#include "../Editor/EditorUndoManager.h"
#include "../Editor/PrefabAsset.h"
#include "../Editor/SceneJsonExporter.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "../math/MathUtil.h"
#include "../scene/Component.h"
#include "../scene/ComponentFactory.h"
#include "../scene/GameObject.h"
#include "../scene/RayCast.h"
#include "../scene/Scene.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace KujakuEngine {
namespace {

constexpr float kDegreesToRadians = 0.017453292519943295f;

struct MaterialInspectorState {
	std::filesystem::path materialPath;
	MaterialAssetData material;
	std::array<char, 256> nameBuffer{};
	std::array<char, 256> baseColorTextureBuffer{};
	std::array<char, 256> normalTextureBuffer{};
	std::array<char, 256> environmentTextureBuffer{};
	std::string errorMessage;
	bool loaded = false;
};

MaterialInspectorState& GetMaterialInspectorState() {
	static MaterialInspectorState state;
	return state;
}

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

float* MatrixData(Matrix4x4& matrix) { return &matrix.m[0][0]; }

const float* MatrixData(const Matrix4x4& matrix) { return &matrix.m[0][0]; }

void CopyTextToBuffer(std::array<char, 256>& buffer, const std::string& text) {
	std::memset(buffer.data(), 0, buffer.size());
	strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
}

void FillMaterialInspectorBuffers(MaterialInspectorState& state) {
	CopyTextToBuffer(state.nameBuffer, state.material.name);
	CopyTextToBuffer(state.baseColorTextureBuffer, MaterialAsset::GetTexturePath(state.material, MaterialTextureSlot::BaseColor));
	CopyTextToBuffer(state.normalTextureBuffer, MaterialAsset::GetTexturePath(state.material, MaterialTextureSlot::Normal));
	CopyTextToBuffer(state.environmentTextureBuffer, MaterialAsset::GetTexturePath(state.material, MaterialTextureSlot::Environment));
}

void ApplyMaterialInspectorBuffers(MaterialInspectorState& state) {
	MaterialAsset::SetTexture(state.material, MaterialTextureSlot::BaseColor, "", state.baseColorTextureBuffer.data());
	MaterialAsset::SetTexture(state.material, MaterialTextureSlot::Normal, "", state.normalTextureBuffer.data());
	MaterialAsset::SetTexture(state.material, MaterialTextureSlot::Environment, "", state.environmentTextureBuffer.data());
}

bool LoadMaterialInspectorState(const std::filesystem::path& materialPath, MaterialInspectorState& state) {
	std::filesystem::path normalizedPath = NormalizeEditorPath(materialPath);
	if (state.loaded && state.materialPath == normalizedPath) {
		return true;
	}

	state.materialPath = normalizedPath;
	state.material = MaterialAsset::CreateDefault();
	state.errorMessage.clear();
	state.loaded = false;

	std::string message;
	if (!MaterialAsset::Load(normalizedPath, state.material, message)) {
		state.errorMessage = message;
		FillMaterialInspectorBuffers(state);
		return false;
	}

	FillMaterialInspectorBuffers(state);
	state.loaded = true;
	return true;
}

bool SaveMaterialInspectorState(MaterialInspectorState& state) {
	std::string message;
	if (!MaterialAsset::Save(state.materialPath, state.material, message)) {
		state.errorMessage = message;
		return false;
	}

	AssetDatabase::GetInstance().GetOrCreateAssetId(state.materialPath);
	state.errorMessage.clear();
	return true;
}

void RefreshMaterialUsers(const std::filesystem::path& materialPath) {
	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		return;
	}

	const std::string materialPathText = materialPath.generic_string();
	for (const std::unique_ptr<GameObject>& gameObject : scene->GetGameObjects()) {
		if (!gameObject) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (!component) {
				continue;
			}
			if (component->UsesMaterialAsset(materialPathText)) {
				component->ApplyMaterialAsset(materialPathText);
			}
		}
	}
}

void DrawMaterialAssetInspector(ProjectWindow& projectWindow) {
	EditorSelection* selection = EditorSelection::GetInstance();
	std::filesystem::path materialPath = selection->GetSelectedAssetPath();
	MaterialInspectorState& state = GetMaterialInspectorState();

	LoadMaterialInspectorState(materialPath, state);

	ImGui::TextUnformatted("Material");
	ImGui::Separator();

	if (!state.errorMessage.empty()) {
		ImGui::TextDisabled("%s", state.errorMessage.c_str());
	}

	ImGui::InputText("Name", state.nameBuffer.data(), state.nameBuffer.size());
	ImGui::SameLine();
	if (ImGui::Button("Apply")) {
		std::filesystem::path renamedPath;
		std::string message;
		if (MaterialAsset::Rename(state.materialPath, state.nameBuffer.data(), renamedPath, message)) {
			selection->SetSelectedAsset(renamedPath, AssetType::Material);
			RefreshMaterialUsers(renamedPath);
			state.materialPath.clear();
			state.loaded = false;
			projectWindow.Refresh();
		} else {
			state.errorMessage = message;
		}
	}

	bool changed = false;
	if (ImGui::ColorEdit4("Base Color", &state.material.baseColor.x)) {
		changed = true;
	}

	if (ImGui::InputText("BaseColor Texture", state.baseColorTextureBuffer.data(), state.baseColorTextureBuffer.size())) {
		MaterialAsset::SetTexture(state.material, MaterialTextureSlot::BaseColor, "", state.baseColorTextureBuffer.data());
		changed = true;
	}

	if (ImGui::InputText("Normal Texture", state.normalTextureBuffer.data(), state.normalTextureBuffer.size())) {
		MaterialAsset::SetTexture(state.material, MaterialTextureSlot::Normal, "", state.normalTextureBuffer.data());
		changed = true;
	}

	if (ImGui::InputText("Environment Texture", state.environmentTextureBuffer.data(), state.environmentTextureBuffer.size())) {
		MaterialAsset::SetTexture(state.material, MaterialTextureSlot::Environment, "", state.environmentTextureBuffer.data());
		changed = true;
	}

	if (changed) {
		if (SaveMaterialInspectorState(state)) {
			RefreshMaterialUsers(state.materialPath);
		}
	}

	if (ImGui::Button("Save")) {
		ApplyMaterialInspectorBuffers(state);
		if (SaveMaterialInspectorState(state)) {
			RefreshMaterialUsers(state.materialPath);
		}
	}

	AssetDatabase& assetDatabase = AssetDatabase::GetInstance();
	std::string assetId = assetDatabase.GetOrCreateAssetId(state.materialPath);
	ImGui::Separator();
	ImGui::TextWrapped("Asset ID: %s", assetId.c_str());
	ImGui::TextWrapped("Path: %s", assetDatabase.MakeProjectRelativePath(state.materialPath).c_str());
	if (ImGui::Button("Copy Asset ID")) {
		ImGui::SetClipboardText(assetId.c_str());
	}
}

GameObject* CreateHierarchyObject(Scene& scene, const char* typeName, GameObject* parent) {
	CaptureUndo(scene, std::string("Create ") + typeName);

	GameObject* created = nullptr;
	if (std::strcmp(typeName, "Entity") == 0) {
		created = scene.CreateEditorEntity();
	} else if (std::strcmp(typeName, "Cube") == 0) {
		created = scene.CreateEditorCube();
	} else if (std::strcmp(typeName, "Sphere") == 0) {
		created = scene.CreateEditorSphere();
	}

	if (!created) {
		return nullptr;
	}

	if (parent) {
		created->SetParent(parent);
	}
	EditorSelection::GetInstance()->SetSelectedGameObject(created);
	return created;
}

void DrawHierarchyCreateMenu(Scene& scene, GameObject* parent) {
	if (!ImGui::BeginMenu("Create")) {
		return;
	}

	if (ImGui::MenuItem("Entity")) {
		CreateHierarchyObject(scene, "Entity", parent);
	}
	if (ImGui::MenuItem("Cube")) {
		CreateHierarchyObject(scene, "Cube", parent);
	}
	if (ImGui::MenuItem("Sphere")) {
		CreateHierarchyObject(scene, "Sphere", parent);
	}

	ImGui::EndMenu();
}

void SaveHierarchyObjectAsPrefab(GameObject* gameObject) {
	if (!gameObject) {
		return;
	}

	PrefabAsset::SaveResult result = PrefabAsset::SaveAsPrefab(*gameObject, DetectEditorProjectRoot());
	if (result.succeeded) {
		PrefabAsset::BindHierarchyToPrefab(*gameObject, result.outputPath);
		ImGuiManager::GetInstance()->AddConsoleLog("[Prefab] Saved: " + result.outputPath.string());
	} else {
		ImGuiManager::GetInstance()->AddConsoleLog("[Prefab] Save failed: " + result.message);
	}
}

bool CanDropHierarchyObject(GameObject* dragged, GameObject* targetParent) {
	if (!dragged) {
		return false;
	}
	if (dragged == targetParent) {
		return false;
	}
	if (targetParent && targetParent->IsDescendantOf(dragged)) {
		return false;
	}
	if (dragged->GetParent() == targetParent) {
		return false;
	}

	return true;
}

bool SceneContainsGameObject(Scene& scene, GameObject* target) {
	if (!target) {
		return false;
	}

	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (gameObject.get() == target) {
			return true;
		}
	}

	return false;
}

bool ApplyMaterialToGameObject(GameObject* gameObject, const std::filesystem::path& materialPath) {
	if (!gameObject) {
		return false;
	}

	bool applied = false;
	for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
		if (!component) {
			continue;
		}
		if (component->ApplyMaterialAsset(materialPath.generic_string())) {
			applied = true;
			break;
		}
	}

	if (!applied) {
		ImGuiManager::GetInstance()->AddConsoleLog("[Material] Target has no ModelRendererComponent: " + gameObject->GetName());
		return false;
	}

	EditorSelection::GetInstance()->SetSelectedGameObject(gameObject);
	ImGuiManager::GetInstance()->AddConsoleLog("[Material] Assigned: " + materialPath.string());
	return true;
}

void AcceptMaterialDropForComponent(Component* component, GameObject* owner) {
	if (!component) {
		return;
	}
	if (!ImGui::BeginDragDropTarget()) {
		return;
	}

	const ImGuiPayload* materialPayload = ImGui::AcceptDragDropPayload(kProjectMaterialDragPayloadType);
	if (materialPayload && materialPayload->DataSize > 0) {
		const char* materialPathText = static_cast<const char*>(materialPayload->Data);
		if (component->ApplyMaterialAsset(materialPathText)) {
			if (owner) {
				EditorSelection::GetInstance()->SetSelectedGameObject(owner);
			}
			ImGuiManager::GetInstance()->AddConsoleLog("[Material] Assigned: " + std::string(materialPathText));
		}
	}

	ImGui::EndDragDropTarget();
}

void DeleteSelectedHierarchyObject(Scene& scene) {
	GameObject* selectedObject = EditorSelection::GetInstance()->GetSelectedGameObject();
	if (!SceneContainsGameObject(scene, selectedObject)) {
		EditorSelection::GetInstance()->Clear();
		return;
	}

	std::string objectName = selectedObject->GetName();
	CaptureUndo(scene, "Delete GameObject");
	EditorSelection::GetInstance()->Clear();
	scene.RemoveGameObjectHierarchy(selectedObject);
	ImGuiManager::GetInstance()->AddConsoleLog("[Hierarchy] Deleted: " + objectName);
}

void AcceptHierarchyObjectDrop(Scene& scene, GameObject* targetParent) {
	if (!ImGui::BeginDragDropTarget()) {
		return;
	}

	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kHierarchyDragPayloadType);
	if (payload && payload->DataSize == sizeof(GameObject*)) {
		GameObject* dragged = *static_cast<GameObject**>(payload->Data);
		if (CanDropHierarchyObject(dragged, targetParent)) {
			CaptureUndo(scene, "Set Parent");
			dragged->SetParent(targetParent, true);
			EditorSelection::GetInstance()->SetSelectedGameObject(dragged);
		}
	}

	const ImGuiPayload* prefabPayload = ImGui::AcceptDragDropPayload(kProjectPrefabDragPayloadType);
	if (prefabPayload && prefabPayload->DataSize > 0) {
		const char* prefabPathText = static_cast<const char*>(prefabPayload->Data);
		CaptureUndo(scene, "Instantiate Prefab");
		GameObject* created = scene.InstantiatePrefab(std::filesystem::path(prefabPathText));
		if (created) {
			if (targetParent) {
				created->SetParent(targetParent);
			}
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}
	}

	const ImGuiPayload* materialPayload = ImGui::AcceptDragDropPayload(kProjectMaterialDragPayloadType);
	if (materialPayload && materialPayload->DataSize > 0) {
		const char* materialPathText = static_cast<const char*>(materialPayload->Data);
		CaptureUndo(scene, "Assign Material");
		ApplyMaterialToGameObject(targetParent, std::filesystem::path(materialPathText));
	}

	ImGui::EndDragDropTarget();
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

	EditorStyle::Apply();
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
	ClearConsoleLogs();
	AddConsoleLog("Console initialized.");
#endif // USE_IMGUI
}

void ImGuiManager::AddConsoleLog(const std::string& message) { EditorConsole::GetInstance()->AddLog(message); }

void ImGuiManager::ClearConsoleLogs() { EditorConsole::GetInstance()->ClearLogs(); }

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
		if (ImGui::Button("Reload DLL")) {
			EditorApplication::GetInstance()->ReloadGameModule();
		}
		ImGui::SameLine();

		if (EditorApplication::GetInstance()->IsPrefabEditing()) {
			if (ImGui::Button("Save Prefab")) {
				EditorApplication::GetInstance()->SavePrefabEditMode();
			}
			ImGui::SameLine();
			if (ImGui::Button("Back")) {
				EditorApplication::GetInstance()->ClosePrefabEditMode(false);
			}
			ImGui::SameLine();
		}

		// 現在のモードをメニューバー上にも出して、Start/Stopの結果をすぐ確認できるようにする。
		if (EditorApplication::GetInstance()->IsPrefabEditing()) {
			ImGui::Text("Mode: PrefabEdit (%s)", EditorApplication::GetInstance()->GetPrefabEditPath().filename().string().c_str());
		} else if (EditorApplication::GetInstance()->IsPlaying()) {
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

void ImGuiManager::HandleGameWindowObjectSelection(const ImVec2& imagePosition, const ImVec2& imageSize) {
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
	HandleGameWindowObjectSelection(imagePosition, imageSize);
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
		if (!object->IsRoot()) {
			continue;
		}

		DrawHierarchyObject(*scene, object.get(), selectedObject, selectedObjectExists);
	}

	ImVec2 remainingRegion = ImGui::GetContentRegionAvail();
	if (remainingRegion.x > 0.0f && remainingRegion.y > 0.0f) {
		ImGui::InvisibleButton("##HierarchyRootDropArea", remainingRegion);
		AcceptHierarchyObjectDrop(*scene, nullptr);
		if (ImGui::BeginPopupContextItem("HierarchyRootDropAreaMenu", ImGuiPopupFlags_MouseButtonRight)) {
			DrawHierarchyCreateMenu(*scene, nullptr);
			ImGui::EndPopup();
		}
	}

	if (ImGui::BeginPopupContextWindow("HierarchyCreateMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
		DrawHierarchyCreateMenu(*scene, nullptr);
		ImGui::EndPopup();
	}

	if (selectedObject && !selectedObjectExists) {
		EditorSelection::GetInstance()->Clear();
	}

	ImGuiIO& io = ImGui::GetIO();
	bool hierarchyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	bool deletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
	if (hierarchyFocused && deletePressed && !io.WantTextInput) {
		DeleteSelectedHierarchyObject(*scene);
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawHierarchyObject(Scene& scene, GameObject* gameObject, GameObject* selectedObject, bool& selectedObjectExists) {
#ifdef USE_IMGUI
	if (!gameObject) {
		return;
	}

	if (gameObject == selectedObject) {
		selectedObjectExists = true;
	}

	std::string displayName = gameObject->GetName();
	if (displayName.empty()) {
		displayName = "(unnamed)";
	}
	if (!gameObject->IsActive()) {
		displayName = "[Inactive] " + displayName;
	}

	const std::vector<GameObject*>& childRefs = gameObject->GetChildren();
	bool hasChildren = !childRefs.empty();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (selectedObject == gameObject) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (!hasChildren) {
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	ImGui::PushID(gameObject);
	int pushedTextColorCount = 0;
	if (!gameObject->IsActive()) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		++pushedTextColorCount;
	} else if (gameObject->IsPrefabInstance()) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.47f, 0.74f, 0.68f, 1.0f));
		++pushedTextColorCount;
	}

	bool opened = ImGui::TreeNodeEx(displayName.c_str(), flags);
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		EditorSelection::GetInstance()->SetSelectedGameObject(gameObject);
	}

	if (ImGui::BeginDragDropSource()) {
		GameObject* payloadObject = gameObject;
		ImGui::SetDragDropPayload(kHierarchyDragPayloadType, &payloadObject, sizeof(payloadObject));
		ImGui::TextUnformatted(displayName.c_str());
		ImGui::EndDragDropSource();
	}

	AcceptHierarchyObjectDrop(scene, gameObject);

	if (pushedTextColorCount > 0) {
		ImGui::PopStyleColor(pushedTextColorCount);
	}

	if (ImGui::BeginPopupContextItem("HierarchyObjectContext")) {
		EditorSelection::GetInstance()->SetSelectedGameObject(gameObject);
		DrawHierarchyCreateMenu(scene, gameObject);

		if (ImGui::BeginMenu("Prefab")) {
			if (ImGui::MenuItem("Create Prefab")) {
				CaptureUndo(scene, "Create Prefab");
				SaveHierarchyObjectAsPrefab(gameObject);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Hierarchy")) {
			if (gameObject->GetParent()) {
				if (ImGui::MenuItem("Unparent")) {
					CaptureUndo(scene, "Unparent");
					gameObject->SetParent(nullptr, true);
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::MenuItem("Unparent");
				ImGui::EndDisabled();
			}
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}

	if (hasChildren && opened) {
		std::vector<GameObject*> children = childRefs;
		for (GameObject* child : children) {
			DrawHierarchyObject(scene, child, selectedObject, selectedObjectExists);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
#else
	(void)scene;
	(void)gameObject;
	(void)selectedObject;
	(void)selectedObjectExists;
#endif // USE_IMGUI
}

void ImGuiManager::DrawInspectorWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Inspector");

	EditorSelection* selection = EditorSelection::GetInstance();
	if (selection->GetSelectedAssetType() == AssetType::Material) {
		DrawMaterialAssetInspector(projectWindow_);
		ImGui::End();
		return;
	}

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		EditorSelection::GetInstance()->Clear();
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	GameObject* selected = selection->GetSelectedGameObject();
	if (!selected) {
		ImGui::TextDisabled("No GameObject selected.");
		ImGui::End();
		return;
	}

	std::string inspectorBeforeJson = scene->ToJson();

	std::array<char, 128> nameBuffer{};
	std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", selected->GetName().c_str());
	if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
		selected->SetName(nameBuffer.data());
	}

	std::array<char, 128> tagBuffer{};
	std::snprintf(tagBuffer.data(), tagBuffer.size(), "%s", selected->GetTag().c_str());
	ImGui::SetNextItemWidth(125.0f);
	if (ImGui::InputText("Tag", tagBuffer.data(), tagBuffer.size())) {
		selected->SetTag(tagBuffer.data());
	}
	ImGui::SameLine();
	int layer = static_cast<int>(selected->GetLayer());
	ImGui::SetNextItemWidth(24.0f);
	if (ImGui::DragInt("Layer", &layer, 1.0f, 0, 31)) {
		if (layer < 0) {
			layer = 0;
		}
		if (layer > 31) {
			layer = 31;
		}
		selected->SetLayer(static_cast<uint32_t>(layer));
	}

	bool active = selected->IsActive();
	if (ImGui::Checkbox("Active", &active)) {
		selected->SetActive(active);
	}

	if (selected->IsPrefabInstance()) {
		ImGui::Separator();
		GameObject* prefabRoot = PrefabAsset::FindPrefabInstanceRoot(*scene, *selected);
		ImGui::TextColored(ImVec4(0.47f, 0.74f, 0.68f, 1.0f), "Prefab Instance");
		ImGui::TextWrapped("%s", selected->GetPrefabAssetPath().c_str());

		if (prefabRoot && prefabRoot != selected) {
			if (ImGui::Button("Select Prefab Root")) {
				EditorSelection::GetInstance()->SetSelectedGameObject(prefabRoot);
			}
		}

		if (ImGui::Button("Open Prefab")) {
			GameObject* openRoot = prefabRoot;
			if (!openRoot) {
				openRoot = selected;
			}
			EditorApplication::GetInstance()->OpenPrefabEditMode(openRoot->GetPrefabAssetPath());
		}
		ImGui::SameLine();
		if (ImGui::Button("Apply")) {
			PrefabAsset::SaveResult result = PrefabAsset::ApplyPrefabInstance(*scene, *selected);
			if (result.succeeded) {
				AddConsoleLog("[Prefab] Applied: " + result.outputPath.string());
				projectWindow_.Refresh();
			} else {
				AddConsoleLog("[Prefab] Apply failed: " + result.message);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert")) {
			PrefabAsset::InstantiateResult result = PrefabAsset::RevertPrefabInstance(*scene, *selected);
			if (result.succeeded) {
				EditorSelection::GetInstance()->SetSelectedGameObject(result.rootObject);
				AddConsoleLog("[Prefab] Reverted.");
			} else {
				AddConsoleLog("[Prefab] Revert failed: " + result.message);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Unpack")) {
			if (PrefabAsset::UnpackPrefabInstance(*scene, *selected)) {
				AddConsoleLog("[Prefab] Unpacked.");
			}
		}
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
			AcceptMaterialDropForComponent(component.get(), selected);

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

	if (inspectorBeforeJson != scene->ToJson() && !inspectorEditing_) {
		CaptureUndo(*scene, "Inspector", inspectorBeforeJson);
		inspectorEditing_ = true;
	}
	if (!ImGui::IsAnyItemActive()) {
		inspectorEditing_ = false;
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawConsoleWindow() { EditorConsole::GetInstance()->Draw(); }

void ImGuiManager::DrawProjectWindow() {
#ifdef USE_IMGUI
	// ProjectDir以下のフォルダ/ファイルを閲覧するProject Windowを描画する。
	projectWindow_.Draw();
#endif // USE_IMGUI
}

void ImGuiManager::HandleEditorShortcuts() {
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontFromFileTTF(
	//	"C:/Windows/Fonts/Arial.ttf", 
	//	24.0f, 
	//	nullptr, 
	//	io.Fonts->GetGlyphRangesJapanese()
	//);

	io.FontGlobalScale = 1.0f;

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (scene && !io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
		bool redo = io.KeyShift;
		if (redo) {
			if (EditorUndoManager::GetInstance()->Redo(*scene)) {
				AddConsoleLog("[Undo] Redo.");
			}
		} else {
			if (EditorUndoManager::GetInstance()->Undo(*scene)) {
				AddConsoleLog("[Undo] Undo.");
			}
		}
		return;
	}
	if (scene && !io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
		if (EditorUndoManager::GetInstance()->Redo(*scene)) {
			AddConsoleLog("[Undo] Redo.");
		}
		return;
	}
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
		if (EditorApplication::GetInstance()->IsPrefabEditing()) {
			EditorApplication::GetInstance()->SavePrefabEditMode();
			return;
		}
		ExportCurrentSceneJson();
	}
	if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_R, false)) {
		EditorApplication::GetInstance()->ReloadGameModule();
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
