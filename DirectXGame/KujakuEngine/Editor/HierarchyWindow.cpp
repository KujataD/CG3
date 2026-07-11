#include "HierarchyWindow.h"

#include "../../externals/imgui/imgui.h"
#include "../scene/Component.h"
#include "../scene/GameObject.h"
#include "../scene/IMaterialTarget.h"
#include "../scene/Scene.h"
#include "EditorApplication.h"
#include "EditorConsole.h"
#include "EditorImGuiUtil.h"
#include "EditorProjectPath.h"
#include "EditorSelection.h"
#include "PrefabAsset.h"
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace KujakuEngine {
namespace {

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
		EditorConsole::GetInstance()->AddLog("[Prefab] Saved: " + result.outputPath.string());
	} else {
		EditorConsole::GetInstance()->AddLog("[Prefab] Save failed: " + result.message);
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
		IMaterialTarget* materialTarget = dynamic_cast<IMaterialTarget*>(component.get());
		if (materialTarget && materialTarget->ApplyMaterialAsset(materialPath.generic_string())) {
			applied = true;
			break;
		}
	}

	if (!applied) {
		EditorConsole::GetInstance()->AddLog("[Material] Target has no ModelRendererComponent: " + gameObject->GetName());
		return false;
	}

	EditorSelection::GetInstance()->SetSelectedGameObject(gameObject);
	EditorConsole::GetInstance()->AddLog("[Material] Assigned: " + materialPath.string());
	return true;
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
	EditorConsole::GetInstance()->AddLog("[Hierarchy] Deleted: " + objectName);
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

void HierarchyWindow::DrawObject(Scene& scene, GameObject* gameObject, GameObject* selectedObject, bool& selectedObjectExists) {
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
			DrawObject(scene, child, selectedObject, selectedObjectExists);
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

void HierarchyWindow::Draw() {
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

		DrawObject(*scene, object.get(), selectedObject, selectedObjectExists);
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

} // namespace KujakuEngine
