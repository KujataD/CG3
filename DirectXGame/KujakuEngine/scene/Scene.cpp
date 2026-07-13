#include "Scene.h"
#include "IEditorBillboard.h"
#include "ISceneCamera.h"
#include "../3d/Camera.h"
#include "../3d/LineRenderer.h"
#include "../3d/Model.h"
#include "../3d/WorldTransform.h"
#include "../base/Time.h"
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
constexpr Vector4 kCameraFrustumColor = {0.85f, 0.85f, 0.95f, 1.0f};

// グローバル重力(Unity既定に合わせる)。RigidbodyのgravityScale倍で各動的ボディに適用する。
constexpr Vector3 kRigidbodyGravity = {0.0f, -9.81f, 0.0f};

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

// contact は colliderA→colliderB の接触情報(非トリガーで交差中のみ非null)。
// flipNormal=true のとき normal を反転して self 視点(self→other)に揃える。
void NotifyCollisionToComponents(GameObject* owner, ColliderComponent* self, ColliderComponent* other, bool isTrigger, CollisionEventPhase phase, const Contact* contact, bool flipNormal) {
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
		if (contact) {
			collision.normal = flipNormal ? contact->normal * -1.0f : contact->normal;
			collision.penetrationDepth = contact->depth;
			collision.point = contact->point;
		} else {
			collision.point = self->GetWorldCenter();
		}

		if (phase == CollisionEventPhase::Enter) {
			component->OnCollisionEnter(collision);
		} else if (phase == CollisionEventPhase::Stay) {
			component->OnCollisionStay(collision);
		} else if (phase == CollisionEventPhase::Exit) {
			component->OnCollisionExit(collision);
		}
	}
}

void NotifyCollisionPair(ColliderComponent* colliderA, ColliderComponent* colliderB, bool isTrigger, CollisionEventPhase phase, const Contact* contact = nullptr) {
	if (!colliderA || !colliderB) {
		return;
	}

	// contact は colliderA→colliderB 基準。B側へは normal を反転して渡す。
	NotifyCollisionToComponents(colliderA->GetOwner(), colliderA, colliderB, isTrigger, phase, contact, false);
	NotifyCollisionToComponents(colliderB->GetOwner(), colliderB, colliderA, isTrigger, phase, contact, true);
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

// Rigidbodyの物理速度(無ければ0=純粋な静的壁)。
Vector3 PhysicsVelocity(RigidbodyComponent* rigidbody) {
	return rigidbody ? rigidbody->GetVelocity() : Vector3{0.0f, 0.0f, 0.0f};
}

// 押し出し・速度変化の対象になる「動的」ボディか(Rigidbody有 かつ Is Static でない)。
bool IsMovable(RigidbodyComponent* rigidbody) {
	return rigidbody && !rigidbody->IsStatic();
}

// Freeze Position が有効な軸の成分を0にする(移動・速度の凍結)。
Vector3 ApplyPositionFreeze(const RigidbodyComponent* rigidbody, Vector3 value) {
	if (rigidbody->FreezePositionX()) {
		value.x = 0.0f;
	}
	if (rigidbody->FreezePositionY()) {
		value.y = 0.0f;
	}
	if (rigidbody->FreezePositionZ()) {
		value.z = 0.0f;
	}
	return value;
}

// 非トリガーの交差ペアに剛体反発(押し出し+速度反射)を適用する。
// 動的(Rigidbody有・非Static)のみ押し出し/速度変化の対象。キネマティック(Is Static)と
// Rigidbody無しは無限質量として不動だが、キネマティックは自身の速度を相手へ与える。
// contact は colliderA→colliderB 基準(呼び出し側で計算済み)。
void ResolveCollisionResponse(ColliderComponent* colliderA, ColliderComponent* colliderB, const Contact& contact) {
	RigidbodyComponent* rigidbodyA = FindRigidbody(colliderA);
	RigidbodyComponent* rigidbodyB = FindRigidbody(colliderB);

	bool movableA = IsMovable(rigidbodyA);
	bool movableB = IsMovable(rigidbodyB);
	if (!movableA && !movableB) {
		return; // 両方とも不動(静的壁/キネマティック) → 応答なし
	}
	if (contact.depth <= 0.0f) {
		return;
	}

	const Vector3& normal = contact.normal; // A→B の分離方向

	// --- 位置補正(押し出し): 動的なAは-normal、動的なBは+normal。両dynamicは折半、片方のみ動的なら全量。---
	float moveA = 0.0f;
	float moveB = 0.0f;
	if (movableA && movableB) {
		moveA = contact.depth * 0.5f;
		moveB = contact.depth * 0.5f;
	} else if (movableA) {
		moveA = contact.depth;
	} else {
		moveB = contact.depth;
	}

	if (moveA > 0.0f) {
		WorldTransform& transformA = colliderA->GetOwner()->GetTransform();
		transformA.translation_ = transformA.translation_ + ApplyPositionFreeze(rigidbodyA, normal * -moveA);
	}
	if (moveB > 0.0f) {
		WorldTransform& transformB = colliderB->GetOwner()->GetTransform();
		transformB.translation_ = transformB.translation_ + ApplyPositionFreeze(rigidbodyB, normal * moveB);
	}

	// --- 速度反射(相対速度ベース): 動的ボディのみ、相手へ接近している場合に相対速度を反射する。---
	// 相手の物理速度(キネマティックの移動速度含む)を加え戻すことで運動量が伝わる。
	// 相手が静的(velocity 0)のときは従来の絶対速度反射に一致する。Freeze軸の速度は0に固定。
	Vector3 physVelocityA = PhysicsVelocity(rigidbodyA);
	Vector3 physVelocityB = PhysicsVelocity(rigidbodyB);

	if (movableA) {
		Vector3 relative = physVelocityA - physVelocityB;
		if (Dot(relative, normal) > 0.0f) { // AがBへ接近
			rigidbodyA->SetVelocity(ApplyPositionFreeze(rigidbodyA, physVelocityB + ShapeUtil::Reflect(relative, normal) * rigidbodyA->GetBounciness()));
		}
	}
	if (movableB) {
		Vector3 relative = physVelocityB - physVelocityA;
		if (Dot(relative, normal) < 0.0f) { // BがAへ接近
			rigidbodyB->SetVelocity(ApplyPositionFreeze(rigidbodyB, physVelocityA + ShapeUtil::Reflect(relative, normal) * rigidbodyB->GetBounciness()));
		}
	}
}

// Rigidbodyの速度を位置へ積分する。動的ボディには重力(gravityScale倍)を加え、Freeze Position軸を固定する。
// キネマティック(Is Static)は重力・拘束を受けず、自身の速度でそのまま移動する。
void IntegrateRigidbodies(Scene& scene, float deltaTime) {
	if (deltaTime <= 0.0f) {
		return;
	}
	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}
		RigidbodyComponent* rigidbody = gameObject->GetComponent<RigidbodyComponent>();
		if (!rigidbody || !rigidbody->IsEnabled()) {
			continue;
		}
		WorldTransform& transform = gameObject->GetTransform();

		if (rigidbody->IsStatic()) {
			// キネマティック: 重力・拘束なしで自身の速度でそのまま移動。
			transform.translation_ = transform.translation_ + rigidbody->GetVelocity() * deltaTime;
			continue;
		}

		// 動的: 重力を加え、Freeze Position軸の速度を0に固定してから積分する。
		Vector3 velocity = rigidbody->GetVelocity() + kRigidbodyGravity * (rigidbody->GetGravityScale() * deltaTime);
		velocity = ApplyPositionFreeze(rigidbody, velocity);
		rigidbody->SetVelocity(velocity);
		transform.translation_ = transform.translation_ + velocity * deltaTime;
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
	// 出し分けは呼び出し側(RenderViewのdrawEditorOverlays)が担当する。Sceneビューはプレイ中も表示。
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

// カメラの視錘台(フラスタム)をワイヤーフレームで描く。Unityのカメラ選択時のギズモ相当。
// 実far(既定1000)は大きすぎるので、向きが分かる固定サイズ(おおよそ5x5x5に収まる)で描く。
// 断面の縦横比は画面(ウィンドウ)の縦横比に沿わせる。cornersはカメラローカル(+Z前方)で作り
// worldMatrixでワールドへ変換する。
void DrawCameraFrustum(const Camera& camera, const Matrix4x4& worldMatrix, const Vector4& color) {
	const float tanHalfFovY = std::tan(camera.fovAngleY * 0.5f);
	const float screenAspect = static_cast<float>(WinApp::kWindowWidth) / static_cast<float>(WinApp::kWindowHeight);

	// far側の奥行きを固定し、半幅/半高/奥行きのどれかが2.5(=5の半分)を超えたら等倍で縮める。
	const float kHalfBox = 2.5f;
	float farDistance = 2.5f;
	const float farHalfHeight = tanHalfFovY * farDistance;
	const float farHalfWidth = farHalfHeight * screenAspect;
	const float maxExtent = (std::max)(farDistance, (std::max)(farHalfWidth, farHalfHeight));
	if (maxExtent > kHalfBox) {
		farDistance *= kHalfBox / maxExtent;
	}
	const float nearDistance = farDistance * 0.1f; // near平面は小さく(四角錐に近い見た目)

	auto MakePlaneCorners = [&](float distance, Vector3 outCorners[4]) {
		const float halfHeight = tanHalfFovY * distance;
		const float halfWidth = halfHeight * screenAspect;
		const Vector3 localCorners[4] = {
		    {-halfWidth, -halfHeight, distance},
		    {halfWidth, -halfHeight, distance},
		    {halfWidth, halfHeight, distance},
		    {-halfWidth, halfHeight, distance},
		};
		for (int i = 0; i < 4; ++i) {
			outCorners[i] = Transform(localCorners[i], worldMatrix);
		}
	};

	Vector3 nearCorners[4];
	Vector3 farCorners[4];
	MakePlaneCorners(nearDistance, nearCorners);
	MakePlaneCorners(farDistance, farCorners);

	for (int i = 0; i < 4; ++i) {
		const int next = (i + 1) % 4;
		LineRenderer::DrawLine(nearCorners[i], nearCorners[next], color); // near平面
		LineRenderer::DrawLine(farCorners[i], farCorners[next], color);   // far平面
		LineRenderer::DrawLine(nearCorners[i], farCorners[i], color);     // 側面エッジ
	}
}

// 選択中のGameObjectがCameraComponentを持つ場合、そのフラスタムを描く。
void DrawCameraDebugLines(Scene& scene) {
	// 出し分けは呼び出し側(RenderViewのdrawEditorOverlays)が担当する。
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
		const ISceneCamera* sceneCamera = dynamic_cast<const ISceneCamera*>(component.get());
		if (!sceneCamera) {
			continue;
		}
		const Camera* camera = sceneCamera->GetSceneCamera();
		if (!camera) {
			continue;
		}
		// カメラが実際に描画へ使う姿勢(=ローカル回転+ワールド位置。SyncFromOwnerTransform参照)で錐体を描く。
		// オーナーのmatWorld_(親の回転/スケールを含む)を使うとGameビューの見え方とズレるため、
		// カメラ自身の向きに合わせる(親が回転していても錐体は実際の描画と一致する)。
		Matrix4x4 cameraWorld = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camera->rotation_, camera->translation_);
		DrawCameraFrustum(*camera, cameraWorld, kCameraFrustumColor);
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
	// 出し分けは呼び出し側(RenderViewのdrawEditorOverlays)が担当する。
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
	// ゲームロジック(Component::Update)がSetVeloc/移動を行う → 速度積分 → 衝突検出+応答 の順。
	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject && gameObject->IsRoot()) {
			gameObject->UpdateHierarchy();
		}
	}

	IntegrateRigidbodies(*this, Time::GetDeltaTime());

	UpdateWorldTransforms();
	UpdateCollisions();
}

void Scene::RefreshEditorBillboards() { PrepareEditorBillboards(*this); }

void Scene::Draw() {
	UpdateWorldTransforms();

	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject && gameObject->IsRoot()) {
			gameObject->DrawHierarchy();
		}
	}

	DrawEditorBillboards(*this);
	DrawColliderDebugLines(*this);
	DrawCameraDebugLines(*this);
	DrawGrid(*this, Vector4(0.4f, 0.4f, 0.4f, 1.0f));
}

void Scene::PrepareFrame() { UpdateWorldTransforms(); }

void Scene::RenderView(Camera* camera, bool drawEditorOverlays) {
	(void)camera; // モデルのカメラは派生側でApplyRenderCameraToModelRenderers済みの想定。

	for (const std::unique_ptr<GameObject>& gameObject : gameObjects_) {
		if (gameObject && gameObject->IsRoot()) {
			gameObject->DrawHierarchy();
		}
	}

	// 編集用オーバーレイはSceneビューのみ(呼び出し側のdrawEditorOverlaysで制御)。
	if (drawEditorOverlays) {
		DrawEditorBillboards(*this);
		DrawColliderDebugLines(*this);
		DrawCameraDebugLines(*this);
		DrawGrid(*this, Vector4(0.4f, 0.4f, 0.4f, 1.0f));
	}
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

			// 非トリガーは接触情報(A→B)を計算し、物理応答(押し出し+速度反射)を適用する。
			// 同じ接触情報をCollisionイベントのnormal/penetration充填にも使う。
			Contact contact{};
			bool hasContact = false;
			if (!isTrigger) {
				hasContact = colliderA->ComputeContact(*colliderB, contact);
				if (hasContact) {
					ResolveCollisionResponse(colliderA, colliderB, contact);
				}
			}
			const Contact* contactPtr = hasContact ? &contact : nullptr;

			std::string pairKey = MakeCollisionPairKey(*colliderA, *colliderB);
			currentPairStates[pairKey] = {colliderA, colliderB, isTrigger};

			auto previous = collisionPairStates_.find(pairKey);
			if (previous == collisionPairStates_.end()) {
				NotifyCollisionPair(colliderA, colliderB, isTrigger, CollisionEventPhase::Enter, contactPtr);
				continue;
			}

			if (previous->second.isTrigger != isTrigger) {
				NotifyCollisionPair(previous->second.colliderA, previous->second.colliderB, previous->second.isTrigger, CollisionEventPhase::Exit);
				NotifyCollisionPair(colliderA, colliderB, isTrigger, CollisionEventPhase::Enter, contactPtr);
				continue;
			}

			NotifyCollisionPair(colliderA, colliderB, isTrigger, CollisionEventPhase::Stay, contactPtr);
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
