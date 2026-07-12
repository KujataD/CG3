#include "Scene.h"
#include "IEditorBillboard.h"
#include "ISceneCamera.h"
#include "../3d/Camera.h"
#include "../3d/LineRenderer.h"
#include "../3d/Model.h"
#include "../3d/WorldTransform.h"
#include "../runtime/PlayState.h"
#include "../runtime/SelectionProvider.h"
#include "../components/ColliderComponent.h"
#include "../components/RigidbodyComponent.h"
#include "../math/MathUtil.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace KujakuEngine {

namespace {

constexpr float kEditorBillboardScale = 0.45f;
constexpr const char* kEditorBillboardTextureDirectory = "KujakuEngine/resources/images/";
constexpr int32_t kColliderSphereSubdivision = 12;
constexpr float kColliderDebugPi = 3.14159265358979323846f;
constexpr Vector4 kColliderDebugColor = {0.1f, 1.0f, 0.35f, 1.0f};

struct EditorBillboardDrawCache {
	std::unordered_map<std::string, std::unique_ptr<Model>> models;
	std::unordered_map<const Component*, std::unique_ptr<WorldTransform>> transforms;
};

EditorBillboardDrawCache& GetEditorBillboardDrawCache() {
	static EditorBillboardDrawCache cache;
	return cache;
}

enum class CollisionEventPhase {
	Enter,
	Stay,
	Exit,
};

std::string MakeCollisionPairKey(const ColliderComponent& colliderA, const ColliderComponent& colliderB) {
	uint64_t idA = colliderA.GetRuntimeId();
	uint64_t idB = colliderB.GetRuntimeId();
	if (idA > idB) {
		std::swap(idA, idB);
	}

	return std::to_string(idA) + ":" + std::to_string(idB);
}

void NotifyCollisionToComponents(GameObject* owner, ColliderComponent* self, ColliderComponent* other, bool isTrigger, CollisionEventPhase phase) {
	if (!owner || !self || !other) {
		return;
	}

	for (const std::unique_ptr<Component>& component : owner->GetComponents()) {
		if (!component || !component->IsEnabled()) {
			continue;
		}

		if (isTrigger) {
			if (phase == CollisionEventPhase::Enter) {
				component->OnTriggerEnter(other);
			} else if (phase == CollisionEventPhase::Stay) {
				component->OnTriggerStay(other);
			} else if (phase == CollisionEventPhase::Exit) {
				component->OnTriggerExit(other);
			}
			continue;
		}

		Collision collision{};
		collision.self = self;
		collision.other = other;
		collision.selfObject = owner;
		collision.otherObject = other->GetOwner();
		collision.point = self->GetWorldCenter();

		if (phase == CollisionEventPhase::Enter) {
			component->OnCollisionEnter(collision);
		} else if (phase == CollisionEventPhase::Stay) {
			component->OnCollisionStay(collision);
		} else if (phase == CollisionEventPhase::Exit) {
			component->OnCollisionExit(collision);
		}
	}
}

void NotifyCollisionPair(ColliderComponent* colliderA, ColliderComponent* colliderB, bool isTrigger, CollisionEventPhase phase) {
	if (!colliderA || !colliderB) {
		return;
	}

	NotifyCollisionToComponents(colliderA->GetOwner(), colliderA, colliderB, isTrigger, phase);
	NotifyCollisionToComponents(colliderB->GetOwner(), colliderB, colliderA, isTrigger, phase);
}

RigidbodyComponent* FindRigidbody(ColliderComponent* collider) {
	if (!collider) {
		return nullptr;
	}
	GameObject* owner = collider->GetOwner();
	if (!owner) {
		return nullptr;
	}
	return owner->GetComponent<RigidbodyComponent>();
}

// 非トリガーの交差ペアに剛体反発(押し出し+速度反射)を適用する。
// Rigidbodyを持つColliderが動的、持たないColliderはStatic(不動)。両Staticは何もしない。
void ResolveCollisionResponse(ColliderComponent* colliderA, ColliderComponent* colliderB) {
	RigidbodyComponent* rigidbodyA = FindRigidbody(colliderA);
	RigidbodyComponent* rigidbodyB = FindRigidbody(colliderB);
	if (!rigidbodyA && !rigidbodyB) {
		return; // 両方Static
	}

	Contact contact{};
	if (!colliderA->ComputeContact(*colliderB, contact)) {
		return;
	}
	if (contact.depth <= 0.0f) {
		return;
	}

	const Vector3& normal = contact.normal; // A→B の分離方向

	// --- 位置補正(押し出し): Aは-normal、Bは+normal。両dynamicは折半、片方Staticは動く側が全量。---
	float moveA = 0.0f;
	float moveB = 0.0f;
	if (rigidbodyA && rigidbodyB) {
		moveA = contact.depth * 0.5f;
		moveB = contact.depth * 0.5f;
	} else if (rigidbodyA) {
		moveA = contact.depth;
	} else {
		moveB = contact.depth;
	}

	if (rigidbodyA && moveA > 0.0f) {
		WorldTransform& transformA = colliderA->GetOwner()->GetTransform();
		transformA.translation_ = transformA.translation_ - normal * moveA;
	}
	if (rigidbodyB && moveB > 0.0f) {
		WorldTransform& transformB = colliderB->GetOwner()->GetTransform();
		transformB.translation_ = transformB.translation_ + normal * moveB;
	}

	// --- 速度反射: 相手へ接近している法線成分のみ反射し反発係数を掛ける。---
	if (rigidbodyA) {
		Vector3 velocityA = rigidbodyA->GetVelocity();
		if (Dot(velocityA, normal) > 0.0f) { // AがB方向へ接近
			rigidbodyA->SetVelocity(ShapeUtil::Reflect(velocityA, normal) * rigidbodyA->GetBounciness());
		}
	}
	if (rigidbodyB) {
		Vector3 velocityB = rigidbodyB->GetVelocity();
		if (Dot(velocityB, normal) < 0.0f) { // BがA方向へ接近
			rigidbodyB->SetVelocity(ShapeUtil::Reflect(velocityB, normal) * rigidbodyB->GetBounciness());
		}
	}
}

void CollectSceneColliders(Scene& scene, std::vector<ColliderComponent*>& outColliders) {
	outColliders.clear();

	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (!component || !component->IsEnabled()) {
				continue;
			}

			ColliderComponent* collider = dynamic_cast<ColliderComponent*>(component.get());
			if (!collider) {
				continue;
			}

			outColliders.push_back(collider);
		}
	}
}

void DrawAABB(const AABB& aabb, const Vector4& color) {
	Vector3 corners[8] = {
	    {aabb.min.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z},
        {aabb.min.x, aabb.max.y, aabb.min.z},
	    {aabb.min.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.max.x, aabb.max.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.max.z},
	};

	LineRenderer::DrawLine(corners[0], corners[1], color);
	LineRenderer::DrawLine(corners[1], corners[2], color);
	LineRenderer::DrawLine(corners[2], corners[3], color);
	LineRenderer::DrawLine(corners[3], corners[0], color);

	LineRenderer::DrawLine(corners[4], corners[5], color);
	LineRenderer::DrawLine(corners[5], corners[6], color);
	LineRenderer::DrawLine(corners[6], corners[7], color);
	LineRenderer::DrawLine(corners[7], corners[4], color);

	LineRenderer::DrawLine(corners[0], corners[4], color);
	LineRenderer::DrawLine(corners[1], corners[5], color);
	LineRenderer::DrawLine(corners[2], corners[6], color);
	LineRenderer::DrawLine(corners[3], corners[7], color);
}

void DrawOBB(const OBB& obb, const Vector4& color) {
	Vector3 axisX = obb.orientations[0] * obb.size.x;
	Vector3 axisY = obb.orientations[1] * obb.size.y;
	Vector3 axisZ = obb.orientations[2] * obb.size.z;
	Vector3 corners[8] = {
	    obb.center - axisX - axisY - axisZ, obb.center + axisX - axisY - axisZ, obb.center + axisX + axisY - axisZ, obb.center - axisX + axisY - axisZ,
	    obb.center - axisX - axisY + axisZ, obb.center + axisX - axisY + axisZ, obb.center + axisX + axisY + axisZ, obb.center - axisX + axisY + axisZ,
	};

	LineRenderer::DrawLine(corners[0], corners[1], color);
	LineRenderer::DrawLine(corners[1], corners[2], color);
	LineRenderer::DrawLine(corners[2], corners[3], color);
	LineRenderer::DrawLine(corners[3], corners[0], color);

	LineRenderer::DrawLine(corners[4], corners[5], color);
	LineRenderer::DrawLine(corners[5], corners[6], color);
	LineRenderer::DrawLine(corners[6], corners[7], color);
	LineRenderer::DrawLine(corners[7], corners[4], color);

	LineRenderer::DrawLine(corners[0], corners[4], color);
	LineRenderer::DrawLine(corners[1], corners[5], color);
	LineRenderer::DrawLine(corners[2], corners[6], color);
	LineRenderer::DrawLine(corners[3], corners[7], color);
}

void DrawSphere(const Sphere& sphere, const Vector4& color) {
	if (sphere.radius <= 0.0f) {
		return;
	}

	float lonEvery = 2.0f * kColliderDebugPi / static_cast<float>(kColliderSphereSubdivision);
	float latEvery = kColliderDebugPi / static_cast<float>(kColliderSphereSubdivision);

	for (int32_t latIndex = 0; latIndex < kColliderSphereSubdivision; ++latIndex) {
		float lat = -kColliderDebugPi / 2.0f + latEvery * static_cast<float>(latIndex);

		for (int32_t lonIndex = 0; lonIndex < kColliderSphereSubdivision; ++lonIndex) {
			float lon = lonEvery * static_cast<float>(lonIndex);

			Vector3 a{};
			Vector3 b{};
			Vector3 c{};

			a.x = sphere.radius * std::cos(lat) * std::cos(lon) + sphere.center.x;
			a.y = sphere.radius * std::sin(lat) + sphere.center.y;
			a.z = sphere.radius * std::cos(lat) * std::sin(lon) + sphere.center.z;

			b.x = sphere.radius * std::cos(lat + latEvery) * std::cos(lon) + sphere.center.x;
			b.y = sphere.radius * std::sin(lat + latEvery) + sphere.center.y;
			b.z = sphere.radius * std::cos(lat + latEvery) * std::sin(lon) + sphere.center.z;

			c.x = sphere.radius * std::cos(lat) * std::cos(lon + lonEvery) + sphere.center.x;
			c.y = sphere.radius * std::sin(lat) + sphere.center.y;
			c.z = sphere.radius * std::cos(lat) * std::sin(lon + lonEvery) + sphere.center.z;

			LineRenderer::DrawLine(a, b, color);
			LineRenderer::DrawLine(a, c, color);
		}
	}
}

void DrawColliderDebugLines(Scene& scene) {
	if (IsGamePlaying()) {
		return;
	}

	GameObject* selectedObject = GetSelectionProvider().GetSelectedGameObject();
	if (!selectedObject || !selectedObject->IsActiveInHierarchy()) {
		return;
	}
	if (scene.FindGameObjectByInstanceId(selectedObject->GetInstanceId()) != selectedObject) {
		return;
	}

	for (const std::unique_ptr<Component>& component : selectedObject->GetComponents()) {
		if (!component || !component->IsEnabled()) {
			continue;
		}

		ColliderComponent* collider = dynamic_cast<ColliderComponent*>(component.get());
		if (!collider) {
			continue;
		}
		if (collider->GetShapeType() == ColliderShapeType::Sphere) {
			DrawSphere(collider->GetWorldSphere(), kColliderDebugColor);
		} else if (collider->GetShapeType() == ColliderShapeType::Box) {
			BoxColliderComponent* boxCollider = dynamic_cast<BoxColliderComponent*>(collider);
			if (boxCollider && boxCollider->UsesWorldOBB()) {
				DrawOBB(boxCollider->GetWorldOBB(), kColliderDebugColor);
			} else {
				DrawAABB(collider->GetWorldAABB(), kColliderDebugColor);
			}
		}
	}
}

void DrawGrid(Scene& scene, const Vector4& color) {
	const float kGridHalfWidth = 100.0f;
	const uint32_t kSubdivision = 100;
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
		Vector3 lineStart = {-kGridHalfWidth + xIndex * kGridEvery, 0.0f, -kGridHalfWidth};
		Vector3 lineEnd = {-kGridHalfWidth + xIndex * kGridEvery, 0.0f, kGridHalfWidth};

		LineRenderer::DrawLine(lineStart, lineEnd, color);
	}

	for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
		Vector3 lineStart = {-kGridHalfWidth, 0.0f, -kGridHalfWidth + zIndex * kGridEvery};
		Vector3 lineEnd = {kGridHalfWidth, 0.0f, -kGridHalfWidth + zIndex * kGridEvery};

		LineRenderer::DrawLine(lineStart, lineEnd, color);
	}
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
	const IEditorBillboard* billboard = dynamic_cast<const IEditorBillboard*>(component);
	if (!billboard) {
		return;
	}

	// TextureManager::LoadTextureはCommandListを実行するため、描画中ではなく初期化・追加時にPlaneを作る。
	GetOrCreateEditorBillboardModel(billboard->GetEditorBillboardIconName());
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
	if (IsGamePlaying()) {
		return;
	}

	Camera* camera = scene.GetEditorCamera();
	if (!camera) {
		return;
	}

	std::unordered_set<const Component*> activeBillboardComponents;
	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			if (!component || !component->IsEnabled()) {
				continue;
			}
			const IEditorBillboard* billboard = dynamic_cast<const IEditorBillboard*>(component.get());
			if (!billboard) {
				continue;
			}
			const ISceneCamera* sceneCamera = dynamic_cast<const ISceneCamera*>(component.get());
			if (sceneCamera && sceneCamera->GetSceneCamera() == camera) {
				// 表示に使っているCamera自身のアイコンを描くと、視点原点のPlaneが目前でちらつくため描画しない。
				continue;
			}

			// Draw中にテクスチャロードが走るとCommandListの状態が壊れるため、準備済みのPlaneだけ描画する。
			Model* model = FindEditorBillboardModel(billboard->GetEditorBillboardIconName());
			WorldTransform* transform = FindEditorBillboardTransform(component.get());
			if (!model || !transform) {
				continue;
			}

			activeBillboardComponents.insert(component.get());

			// GameObjectのワールド位置へPlaneを置き、WorldTransform側のBillboard行列で常にEditorCameraへ向ける。
			transform->translation_ = gameObject->GetTransform().GetWorldPosition();
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
		if (gameObject && gameObject->IsRoot()) {
			gameObject->UpdateHierarchy();
		}
	}

	UpdateWorldTransforms();
	UpdateCollisions();
}

void Scene::Draw() {
	UpdateWorldTransforms();

	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject && gameObject->IsRoot()) {
			gameObject->DrawHierarchy();
		}
	}

	DrawEditorBillboards(*this);
	DrawColliderDebugLines(*this);
	DrawGrid(*this, Vector4(0.4f, 0.4f, 0.4f, 1.0f));
}

void Scene::Finalize() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject) {
			gameObject->Finalize();
		}
	}
	gameObjects_.clear();
	ClearEditorBillboardDrawCache();
	collisionPairStates_.clear();
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
	collisionPairStates_.clear();
}

void Scene::UpdateCollisions() {
	std::vector<ColliderComponent*> colliders;
	CollectSceneColliders(*this, colliders);

	std::unordered_set<ColliderComponent*> activeColliders;
	for (ColliderComponent* collider : colliders) {
		activeColliders.insert(collider);
	}

	std::unordered_map<std::string, CollisionPairState> currentPairStates;
	for (size_t indexA = 0; indexA < colliders.size(); ++indexA) {
		ColliderComponent* colliderA = colliders[indexA];
		if (!colliderA) {
			continue;
		}

		for (size_t indexB = indexA + 1; indexB < colliders.size(); ++indexB) {
			ColliderComponent* colliderB = colliders[indexB];
			if (!colliderB) {
				continue;
			}
			if (colliderA->GetOwner() == colliderB->GetOwner()) {
				continue;
			}
			if (!colliderA->Intersects(*colliderB)) {
				continue;
			}

			bool isTrigger = colliderA->IsTrigger() || colliderB->IsTrigger();

			// 非トリガーは物理応答(押し出し+速度反射)を適用する。
			if (!isTrigger) {
				ResolveCollisionResponse(colliderA, colliderB);
			}

			std::string pairKey = MakeCollisionPairKey(*colliderA, *colliderB);
			currentPairStates[pairKey] = {colliderA, colliderB, isTrigger};

			auto previous = collisionPairStates_.find(pairKey);
			if (previous == collisionPairStates_.end()) {
				NotifyCollisionPair(colliderA, colliderB, isTrigger, CollisionEventPhase::Enter);
				continue;
			}

			if (previous->second.isTrigger != isTrigger) {
				NotifyCollisionPair(previous->second.colliderA, previous->second.colliderB, previous->second.isTrigger, CollisionEventPhase::Exit);
				NotifyCollisionPair(colliderA, colliderB, isTrigger, CollisionEventPhase::Enter);
				continue;
			}

			NotifyCollisionPair(colliderA, colliderB, isTrigger, CollisionEventPhase::Stay);
		}
	}

	for (const auto& [pairKey, previousState] : collisionPairStates_) {
		if (currentPairStates.contains(pairKey)) {
			continue;
		}
		if (!activeColliders.contains(previousState.colliderA) || !activeColliders.contains(previousState.colliderB)) {
			continue;
		}

		NotifyCollisionPair(previousState.colliderA, previousState.colliderB, previousState.isTrigger, CollisionEventPhase::Exit);
	}

	collisionPairStates_ = std::move(currentPairStates);
}

GameObject* Scene::CreateGameObject(const std::string& name) { return AddGameObject(std::make_unique<GameObject>(name)); }

GameObject* Scene::CreateEditorEntity() { return CreateGameObject("Entity"); }

GameObject* Scene::CreateEditorCube() { return CreateGameObject("Cube"); }

GameObject* Scene::CreateEditorSphere() { return CreateGameObject("Sphere"); }

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

void Scene::RemoveGameObjectHierarchy(GameObject* gameObject) {
	if (!gameObject) {
		return;
	}

	std::vector<GameObject*> removeTargets;
	removeTargets.push_back(gameObject);
	for (size_t index = 0; index < removeTargets.size(); ++index) {
		GameObject* current = removeTargets[index];
		if (!current) {
			continue;
		}
		for (GameObject* child : current->GetChildren()) {
			if (child) {
				removeTargets.push_back(child);
			}
		}
	}

	for (GameObject* target : removeTargets) {
		if (target) {
			target->Finalize();
		}
	}

	auto shouldRemove = [&removeTargets](const std::unique_ptr<GameObject>& current) {
		if (!current) {
			return false;
		}
		return std::find(removeTargets.begin(), removeTargets.end(), current.get()) != removeTargets.end();
	};
	gameObjects_.erase(std::remove_if(gameObjects_.begin(), gameObjects_.end(), shouldRemove), gameObjects_.end());
}

GameObject* Scene::FindGameObjectByInstanceId(const std::string& instanceId) const {
	if (instanceId.empty()) {
		return nullptr;
	}

	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (!gameObject) {
			continue;
		}
		if (gameObject->GetInstanceId() == instanceId) {
			return gameObject.get();
		}
	}

	return nullptr;
}

void Scene::UpdateWorldTransforms() {
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject && gameObject->IsRoot()) {
			gameObject->UpdateWorldTransformHierarchy();
		}
	}
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
		GameObject* parent = gameObject->GetParent();
		std::string parentInstanceId;
		if (parent) {
			parentInstanceId = parent->GetInstanceId();
		}
		os << "      \"parentInstanceId\": \"" << EscapeJsonString(parentInstanceId) << "\",\n";
		os << "      \"prefabAssetPath\": \"" << EscapeJsonString(gameObject->GetPrefabAssetPath()) << "\",\n";
		os << "      \"prefabObjectId\": \"" << EscapeJsonString(gameObject->GetPrefabObjectId()) << "\",\n";
		os << "      \"prefabInstanceRootId\": \"" << EscapeJsonString(gameObject->GetPrefabInstanceRootId()) << "\",\n";
		os << "      \"prefabInstanceRoot\": ";
		if (gameObject->IsPrefabInstanceRoot()) {
			os << "true,\n";
		} else {
			os << "false,\n";
		}
		os << "      \"name\": \"" << EscapeJsonString(gameObject->GetName()) << "\",\n";
		os << "      \"tag\": \"" << EscapeJsonString(gameObject->GetTag()) << "\",\n";
		os << "      \"layer\": " << gameObject->GetLayer() << ",\n";
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
