#include "Scene.h"
#include "../3d/Camera.h"
#include "../3d/Model.h"
#include "../3d/WorldTransform.h"
#include "../Editor/EditorApplication.h"
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace KujakuEngine {

namespace {

constexpr float kEditorBillboardScale = 0.45f;
constexpr const char* kEditorBillboardTextureDirectory = "KujakuEngine/resources/images/";

struct EditorBillboardDrawCache {
	std::unordered_map<std::string, std::unique_ptr<Model>> models;
	std::unordered_map<const Component*, std::unique_ptr<WorldTransform>> transforms;
};

EditorBillboardDrawCache& GetEditorBillboardDrawCache() {
	static EditorBillboardDrawCache cache;
	return cache;
}

std::string EscapeJsonString(const std::string& text) {
	std::string escaped;
	escaped.reserve(text.size());

	for (char character : text) {
		if (character == '\\') {
			escaped += "\\\\";
		} else if (character == '"') {
			escaped += "\\\"";
		} else if (character == '\n') {
			escaped += "\\n";
		} else if (character == '\r') {
			escaped += "\\r";
		} else if (character == '\t') {
			escaped += "\\t";
		} else {
			escaped += character;
		}
	}

	return escaped;
}

Model* GetOrCreateEditorBillboardModel(const std::string& iconName) {
	if (iconName.empty()) {
		return nullptr;
	}

	EditorBillboardDrawCache& cache = GetEditorBillboardDrawCache();
	auto found = cache.models.find(iconName);
	if (found != cache.models.end()) {
		return found->second.get();
	}

	// Editor用アイコンも通常の3Dモデルと同じTexture/Model経路で扱い、GameWindow内の実体あるPlaneとして描画する。
	std::string texturePath = std::string(kEditorBillboardTextureDirectory) + iconName;
	std::unique_ptr<Model> model(Model::CreatePlane(texturePath, ShaderModel::kNone));
	Model* rawModel = model.get();
	cache.models.emplace(iconName, std::move(model));
	return rawModel;
}

Model* FindEditorBillboardModel(const std::string& iconName) {
	EditorBillboardDrawCache& cache = GetEditorBillboardDrawCache();
	auto found = cache.models.find(iconName);
	if (found == cache.models.end()) {
		return nullptr;
	}

	return found->second.get();
}

WorldTransform* GetOrCreateEditorBillboardTransform(const Component* component) {
	if (!component) {
		return nullptr;
	}

	EditorBillboardDrawCache& cache = GetEditorBillboardDrawCache();
	auto found = cache.transforms.find(component);
	if (found != cache.transforms.end()) {
		return found->second.get();
	}

	std::unique_ptr<WorldTransform> transform = std::make_unique<WorldTransform>();
	transform->Initialize();
	WorldTransform* rawTransform = transform.get();
	cache.transforms.emplace(component, std::move(transform));
	return rawTransform;
}

WorldTransform* FindEditorBillboardTransform(const Component* component) {
	EditorBillboardDrawCache& cache = GetEditorBillboardDrawCache();
	auto found = cache.transforms.find(component);
	if (found == cache.transforms.end()) {
		return nullptr;
	}

	return found->second.get();
}

void PrepareEditorBillboardComponent(const Component* component) {
	if (!component || !component->HasEditorBillboard()) {
		return;
	}

	// TextureManager::LoadTextureはCommandListを実行するため、描画中ではなく初期化・追加時にPlaneを作る。
	GetOrCreateEditorBillboardModel(component->GetEditorBillboardIconName());
	GetOrCreateEditorBillboardTransform(component);
}

void PrepareEditorBillboards(Scene& scene) {
	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (!gameObject) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			PrepareEditorBillboardComponent(component.get());
		}
	}
}

void DrawEditorBillboards(Scene& scene) {
	if (EditorApplication::GetInstance()->IsPlaying()) {
		return;
	}

	Camera* camera = scene.GetEditorCamera();
	if (!camera) {
		return;
	}

	std::unordered_set<const Component*> activeBillboardComponents;
	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (!gameObject || !gameObject->IsActive()) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (!component || !component->IsEnabled()) {
				continue;
			}
			if (!component->HasEditorBillboard()) {
				continue;
			}

			// Draw中にテクスチャロードが走るとCommandListの状態が壊れるため、準備済みのPlaneだけ描画する。
			Model* model = FindEditorBillboardModel(component->GetEditorBillboardIconName());
			WorldTransform* transform = FindEditorBillboardTransform(component.get());
			if (!model || !transform) {
				continue;
			}

			activeBillboardComponents.insert(component.get());

			// GameObjectの位置へPlaneを置き、WorldTransform側のBillboard行列で常にEditorCameraへ向ける。
			transform->translation_ = gameObject->GetTransform().translation_;
			transform->rotation_ = {0.0f, 0.0f, 0.0f};
			transform->scale_ = {kEditorBillboardScale, kEditorBillboardScale, kEditorBillboardScale};
			transform->UpdateMatrix(*camera, true);
			model->Draw(*transform, *camera);
		}
	}

	EditorBillboardDrawCache& cache = GetEditorBillboardDrawCache();
	for (auto iterator = cache.transforms.begin(); iterator != cache.transforms.end();) {
		if (activeBillboardComponents.find(iterator->first) == activeBillboardComponents.end()) {
			iterator = cache.transforms.erase(iterator);
		} else {
			++iterator;
		}
	}
}

void ClearEditorBillboardDrawCache() {
	EditorBillboardDrawCache& cache = GetEditorBillboardDrawCache();
	cache.transforms.clear();
	cache.models.clear();
}

} // namespace

Scene::~Scene() = default;

void Scene::Initialize() {
	if (initialized_) {
		return;
	}

	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->Initialize();
		}
	}

	initialized_ = true;
	PrepareEditorBillboards(*this);
}

void Scene::Update() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->Update();
		}
	}
}

void Scene::Draw() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->Draw();
		}
	}

	DrawEditorBillboards(*this);
}

void Scene::Finalize() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->Finalize();
		}
	}
	gameObjects_.clear();
	ClearEditorBillboardDrawCache();
	initialized_ = false;
}

void Scene::OnPlayStart() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->OnPlayStart();
		}
	}
}

void Scene::OnPlayStop() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->OnPlayStop();
		}
	}
}

GameObject* Scene::CreateGameObject(const std::string& name) {
	return AddGameObject(std::make_unique<GameObject>(name));
}

GameObject* Scene::CreateEditorEntity() {
	return CreateGameObject("Entity");
}

GameObject* Scene::CreateEditorCube() {
	return CreateGameObject("Cube");
}

GameObject* Scene::CreateEditorSphere() {
	return CreateGameObject("Sphere");
}

void Scene::OnEditorComponentAdded(GameObject* gameObject, Component* component) {
	(void)gameObject;
	PrepareEditorBillboardComponent(component);
}

GameObject* Scene::AddGameObject(std::unique_ptr<GameObject> gameObject) {
	GameObject* raw = gameObject.get();
	gameObjects_.push_back(std::move(gameObject));

	if (initialized_ && raw) {
		raw->Initialize();
		for (const std::unique_ptr<Component>& component : raw->GetComponents()) {
			PrepareEditorBillboardComponent(component.get());
		}
	}

	return raw;
}

std::string Scene::ToJson() const {
	std::ostringstream os;
	os << "{\n";
	os << "  \"objects\": [\n";

	for (size_t objectIndex = 0; objectIndex < gameObjects_.size(); ++objectIndex) {
		const std::unique_ptr<GameObject>& gameObject = gameObjects_[objectIndex];
		if (!gameObject) {
			continue;
		}

		os << "    {\n";
		os << "      \"instanceId\": \"" << EscapeJsonString(gameObject->GetInstanceId()) << "\",\n";
		os << "      \"name\": \"" << EscapeJsonString(gameObject->GetName()) << "\",\n";
		os << "      \"active\": ";
		if (gameObject->IsActive()) {
			os << "true,\n";
		} else {
			os << "false,\n";
		}
		os << "      \"components\": [\n";

		const std::vector<std::unique_ptr<Component>>& components = gameObject->GetComponents();
		for (size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex) {
			const std::unique_ptr<Component>& component = components[componentIndex];
			if (!component) {
				continue;
			}

			component->WriteJson(os, 8);
			if (componentIndex + 1 < components.size()) {
				os << ",";
			}
			os << "\n";
		}

		os << "      ]\n";
		os << "    }";
		if (objectIndex + 1 < gameObjects_.size()) {
			os << ",";
		}
		os << "\n";
	}

	os << "  ]\n";
	os << "}\n";
	return os.str();
}

} // namespace KujakuEngine
