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

	// 初期レイアウトはImGui初期化後、最初のEditorDockSpace::Drawで構築する。
	dockSpace_.ResetLayout();
	ClearConsoleLogs();
	AddConsoleLog("Console initialized.");
#endif // USE_IMGUI
}

void ImGuiManager::AddConsoleLog(const std::string& message) { EditorConsole::GetInstance()->AddLog(message); }

void ImGuiManager::ClearConsoleLogs() { EditorConsole::GetInstance()->ClearLogs(); }

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
	dockSpace_.Draw();
	sceneView_.Draw(projectWindow_.GetProjectRoot());
	hierarchyWindow_.Draw();
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
