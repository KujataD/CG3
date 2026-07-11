#include "MaterialInspector.h"

#include "../../externals/imgui/imgui.h"
#include "../assets/MaterialAsset.h"
#include "../scene/Component.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include "AssetDatabase.h"
#include "EditorApplication.h"
#include "EditorProjectPath.h"
#include "EditorSelection.h"
#include "ProjectWindow.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

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

} // namespace KujakuEngine
