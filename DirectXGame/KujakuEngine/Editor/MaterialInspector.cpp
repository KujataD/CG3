#include "MaterialInspector.h"

#include "../../externals/imgui/imgui.h"
#include "../../externals/imsearch/imsearch.h"
#include "../assets/MaterialAsset.h"
#include "../scene/Component.h"
#include "../scene/GameObject.h"
#include "../scene/IMaterialTarget.h"
#include "../scene/Scene.h"
#include "AssetDatabase.h"
#include "EditorApplication.h"
#include "EditorImGuiUtil.h"
#include "../base/ProjectPath.h"
#include "EditorSelection.h"
#include "ProjectWindow.h"
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

// OSファイル選択ダイアログ(IFileDialog)用。
#include <Windows.h>
#include <shobjidl.h>
#pragma comment(lib, "ole32.lib")

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

// 選択したテクスチャパスをassetId解決付きでMaterialへ反映し、表示用バッファも更新する。
void ApplyTextureSelection(MaterialInspectorState& state, MaterialTextureSlot slot, std::array<char, 256>& buffer, const std::filesystem::path& texturePath) {
	AssetDatabase& db = AssetDatabase::GetInstance();
	// プロジェクト相対パスを保存パスに、assetIdを安定参照として保存する(従来はpathのみでassetIdが空だった)。
	std::string relativePath = db.MakeProjectRelativePath(texturePath);
	if (relativePath.empty()) {
		relativePath = texturePath.generic_string();
	}
	std::string assetId = db.GetOrCreateAssetId(texturePath);
	MaterialAsset::SetTexture(state.material, slot, assetId, relativePath);
	CopyTextToBuffer(buffer, relativePath);
}

// プロジェクト内の画像ファイル(.png/.jpg/.jpeg)を列挙する。
std::vector<std::filesystem::path> EnumerateProjectTextures() {
	std::vector<std::filesystem::path> textures;
	std::filesystem::path projectRoot = DetectEditorProjectRoot();
	if (projectRoot.empty()) {
		return textures;
	}

	std::error_code errorCode;
	std::filesystem::recursive_directory_iterator iterator(projectRoot, std::filesystem::directory_options::skip_permission_denied, errorCode);
	std::filesystem::recursive_directory_iterator end;
	for (; iterator != end; iterator.increment(errorCode)) {
		if (errorCode) {
			break;
		}
		if (!iterator->is_regular_file(errorCode)) {
			continue;
		}
		std::filesystem::path extension = iterator->path().extension();
		std::string ext = extension.string();
		for (char& character : ext) {
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
		}
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
			textures.push_back(iterator->path());
		}
	}
	return textures;
}

// OSのファイル選択ダイアログで画像ファイルを1つ選ぶ。選択されたらtrueとパスを返す。
bool OpenTextureFileDialog(std::filesystem::path& outPath) {
	HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	const bool shouldUninitialize = SUCCEEDED(initResult);

	bool picked = false;
	IFileOpenDialog* fileDialog = nullptr;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog)))) {
		const COMDLG_FILTERSPEC filters[] = {
		    {L"Image Files", L"*.png;*.jpg;*.jpeg"},
		    {L"All Files", L"*.*"},
		};
		fileDialog->SetFileTypes(static_cast<UINT>(std::size(filters)), filters);

		if (SUCCEEDED(fileDialog->Show(nullptr))) {
			IShellItem* item = nullptr;
			if (SUCCEEDED(fileDialog->GetResult(&item))) {
				PWSTR filePath = nullptr;
				if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &filePath))) {
					outPath = std::filesystem::path(filePath);
					picked = true;
					CoTaskMemFree(filePath);
				}
				item->Release();
			}
		}
		fileDialog->Release();
	}

	if (shouldUninitialize) {
		CoUninitialize();
	}
	return picked;
}

// 1テクスチャスロット分のUI(手動入力 + D&D + 一覧選択 + ファイルダイアログ)。変更があればtrue。
bool DrawTextureSlotEditor(const char* label, const char* popupId, MaterialInspectorState& state, MaterialTextureSlot slot, std::array<char, 256>& buffer) {
	bool changed = false;
	ImGui::PushID(label);

	// 手動パス入力も残す(直接編集したい場合)。assetIdはクリアされ、pathのみになる。
	ImGui::SetNextItemWidth(-160.0f);
	if (ImGui::InputText(label, buffer.data(), buffer.size())) {
		MaterialAsset::SetTexture(state.material, slot, "", buffer.data());
		changed = true;
	}

	// Projectからのテクスチャドロップを受け付ける。
	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kProjectTextureDragPayloadType);
		if (payload && payload->DataSize > 0) {
			ApplyTextureSelection(state, slot, buffer, std::filesystem::path(static_cast<const char*>(payload->Data)));
			changed = true;
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::SameLine();
	if (ImGui::Button("Select")) {
		ImGui::OpenPopup(popupId);
	}
	ImGui::SameLine();
	if (ImGui::Button("...")) {
		std::filesystem::path picked;
		if (OpenTextureFileDialog(picked)) {
			ApplyTextureSelection(state, slot, buffer, picked);
			changed = true;
		}
	}

	// プロジェクト内テクスチャ一覧(ImSearchで絞り込み)。
	if (ImGui::BeginPopup(popupId)) {
		if (ImSearch::BeginSearch()) {
			ImSearch::SearchBar("Search Texture");
			AssetDatabase& db = AssetDatabase::GetInstance();
			for (const std::filesystem::path& texturePath : EnumerateProjectTextures()) {
				std::string relative = db.MakeProjectRelativePath(texturePath);
				if (relative.empty()) {
					relative = texturePath.generic_string();
				}
				ImSearch::SearchableItem(relative.c_str(), [&](const char* name) {
					if (ImGui::Selectable(name)) {
						ApplyTextureSelection(state, slot, buffer, texturePath);
						changed = true;
						ImGui::CloseCurrentPopup();
					}
				});
			}
			ImSearch::EndSearch();
		}
		ImGui::EndPopup();
	}

	ImGui::PopID();
	return changed;
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
			IMaterialTarget* materialTarget = dynamic_cast<IMaterialTarget*>(component.get());
			if (materialTarget && materialTarget->UsesMaterialAsset(materialPathText)) {
				materialTarget->ApplyMaterialAsset(materialPathText);
			}
		}
	}
}

} // namespace

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

	if (DrawTextureSlotEditor("BaseColor Texture", "BaseColorTexturePicker", state, MaterialTextureSlot::BaseColor, state.baseColorTextureBuffer)) {
		changed = true;
	}

	if (DrawTextureSlotEditor("Normal Texture", "NormalTexturePicker", state, MaterialTextureSlot::Normal, state.normalTextureBuffer)) {
		changed = true;
	}

	if (DrawTextureSlotEditor("Environment Texture", "EnvironmentTexturePicker", state, MaterialTextureSlot::Environment, state.environmentTextureBuffer)) {
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

} // namespace KujakuEngine
