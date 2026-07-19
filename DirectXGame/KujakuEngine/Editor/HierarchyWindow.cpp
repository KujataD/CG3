#include "HierarchyWindow.h"

#include "../../externals/imgui/imgui.h"
#include "../scene/Component.h"
#include "../scene/GameObject.h"
#include "../scene/IMaterialTarget.h"
#include "../scene/Scene.h"
#include "EditorApplication.h"
#include "EditorConsole.h"
#include "EditorImGuiUtil.h"
#include "../base/ProjectPath.h"
#include "EditorSelection.h"
#include "PrefabAsset.h"
#include "UIObjectFactory.h"
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

	// UIはGameObjectメニューと共有(UIObjectFactory)。Canvas配下に生成する。
	ImGui::Separator();
	if (ImGui::BeginMenu("UI")) {
		if (ImGui::MenuItem("Canvas")) {
			CaptureUndo(scene, "Create Canvas");
			EditorSelection::GetInstance()->SetSelectedGameObject(UIObjectFactory::EnsureCanvas(&scene));
		}
		if (ImGui::MenuItem("Image")) {
			CaptureUndo(scene, "Create UI Image");
			EditorSelection::GetInstance()->SetSelectedGameObject(UIObjectFactory::CreateImage(&scene));
		}
		if (ImGui::MenuItem("Text")) {
			CaptureUndo(scene, "Create UI Text");
			EditorSelection::GetInstance()->SetSelectedGameObject(UIObjectFactory::CreateText(&scene));
		}
		if (ImGui::MenuItem("Button")) {
			CaptureUndo(scene, "Create UI Button");
			EditorSelection::GetInstance()->SetSelectedGameObject(UIObjectFactory::CreateButton(&scene));
		}
		ImGui::EndMenu();
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
		GameObject* created = PrefabAsset::Instantiate(scene, std::filesystem::path(prefabPathText)).rootObject;
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

// ノード上のドロップ位置を判定して、並び替え(前/後)と親子化(中央)を切り替える。
// 上30%=targetの直前へ / 下30%=targetの直後へ / 中40%=targetの子にする。
void AcceptHierarchyObjectDropOnNode(Scene& scene, GameObject* target, const ImVec2& itemMin, const ImVec2& itemMax) {
	if (!ImGui::BeginDragDropTarget()) {
		return;
	}

	// ドラッグ中ペイロードを覗いて、Hierarchyのドラッグなら挿入インジケータを描く。
	const ImGuiPayload* peekPayload = ImGui::GetDragDropPayload();
	const bool isHierarchyDrag = peekPayload && peekPayload->IsDataType(kHierarchyDragPayloadType);

	// 0=直前へ, 1=子にする, 2=直後へ
	int dropZone = 1;
	if (isHierarchyDrag) {
		const float height = itemMax.y - itemMin.y;
		const float ratio = height > 0.0f ? (ImGui::GetMousePos().y - itemMin.y) / height : 0.5f;
		if (ratio < 0.30f) {
			dropZone = 0;
		} else if (ratio > 0.70f) {
			dropZone = 2;
		} else {
			dropZone = 1;
		}

		if (dropZone != 1) {
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
			const float lineY = (dropZone == 0) ? itemMin.y : itemMax.y;
			drawList->AddLine(ImVec2(itemMin.x, lineY), ImVec2(itemMax.x, lineY), lineColor, 2.0f);
		}
		// dropZone==1(子にする)はImGui標準の矩形ハイライトに任せる。
	}

	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kHierarchyDragPayloadType);
	if (payload && payload->DataSize == sizeof(GameObject*)) {
		GameObject* dragged = *static_cast<GameObject**>(payload->Data);
		if (dropZone == 1) {
			if (CanDropHierarchyObject(dragged, target)) {
				CaptureUndo(scene, "Set Parent");
				dragged->SetParent(target, true);
				EditorSelection::GetInstance()->SetSelectedGameObject(dragged);
			}
		} else {
			CaptureUndo(scene, "Reorder GameObject");
			if (scene.MoveGameObjectOrder(dragged, target, dropZone == 2)) {
				EditorSelection::GetInstance()->SetSelectedGameObject(dragged);
			}
		}
	}

	// Prefab/Materialのドロップはノードへの適用(中央相当)として従来どおり受け付ける。
	const ImGuiPayload* prefabPayload = ImGui::AcceptDragDropPayload(kProjectPrefabDragPayloadType);
	if (prefabPayload && prefabPayload->DataSize > 0) {
		const char* prefabPathText = static_cast<const char*>(prefabPayload->Data);
		CaptureUndo(scene, "Instantiate Prefab");
		GameObject* created = PrefabAsset::Instantiate(scene, std::filesystem::path(prefabPathText)).rootObject;
		if (created) {
			if (target) {
				created->SetParent(target);
			}
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}
	}

	const ImGuiPayload* materialPayload = ImGui::AcceptDragDropPayload(kProjectMaterialDragPayloadType);
	if (materialPayload && materialPayload->DataSize > 0) {
		const char* materialPathText = static_cast<const char*>(materialPayload->Data);
		CaptureUndo(scene, "Assign Material");
		ApplyMaterialToGameObject(target, std::filesystem::path(materialPathText));
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
	// 並び替えのドロップ位置判定に使うノードの矩形を、この時点で取得しておく。
	const ImVec2 itemMin = ImGui::GetItemRectMin();
	const ImVec2 itemMax = ImGui::GetItemRectMax();
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		EditorSelection::GetInstance()->SetSelectedGameObject(gameObject);
	}

	if (ImGui::BeginDragDropSource()) {
		GameObject* payloadObject = gameObject;
		ImGui::SetDragDropPayload(kHierarchyDragPayloadType, &payloadObject, sizeof(payloadObject));
		ImGui::TextUnformatted(displayName.c_str());
		ImGui::EndDragDropSource();
	}

	AcceptHierarchyObjectDropOnNode(scene, gameObject, itemMin, itemMax);

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

void HierarchyWindow::Draw(bool* pOpen) {
#ifdef USE_IMGUI
	ImGui::Begin("Hierarchy", pOpen);

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		EditorSelection::GetInstance()->Clear();
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	GameObject* selectedObject = EditorSelection::GetInstance()->GetSelectedGameObject();
	bool selectedObjectExists = false;

	// DrawObject内のD&D(並び替え=MoveGameObjectOrder / Prefab生成など)はgameObjects_を
	// 変更・再確保するため、range-forの参照が無効化されてクラッシュする。
	// 子ループ(DrawObject内)と同様に、描画前にルートのポインタをスナップショットしてから反復する。
	std::vector<GameObject*> rootObjects;
	for (const std::unique_ptr<GameObject>& object : scene->GetGameObjects()) {
		if (!object) {
			continue;
		}
		if (!object->IsRoot()) {
			continue;
		}
		rootObjects.push_back(object.get());
	}

	for (GameObject* rootObject : rootObjects) {
		DrawObject(*scene, rootObject, selectedObject, selectedObjectExists);
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
#else
	(void)pOpen;
#endif // USE_IMGUI
}

} // namespace KujakuEngine
