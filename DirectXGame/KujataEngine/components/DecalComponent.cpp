#include "DecalComponent.h"

#include "../3d/Camera.h"
#include "../base/ProjectPath.h"
#include "../components/ColliderComponent.h"
#include "../components/RigidbodyComponent.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>

namespace KujataEngine {

namespace {

/// <summary>
/// 足場として扱ってよいコライダーか。
/// **まずコライダー自身の明示指定(Moving Collider)を見る。**
/// Rigidbodyの有無から推測する方法は、部位ごとに当たり判定を分けた相手で当たらない
/// (部位側はRigidbodyを持たず、ルートはコードで動かす都合でIs Staticになっている)。
/// </summary>
bool IsGroundCollider(GameObject& gameObject, const ColliderComponent& collider) {
	if (collider.IsMovingCollider()) {
		return false;
	}
	RigidbodyComponent* rigidbody = gameObject.GetComponent<RigidbodyComponent>();
	return !rigidbody || rigidbody->IsStatic();
}

/// <summary>objectがrootの子孫(またはroot自身)か。自分の体を地面と誤認しないための除外に使う。</summary>
bool IsSelfOrDescendantOf(const GameObject* object, const GameObject* root) {
	for (const GameObject* current = object; current; current = current->GetParent()) {
		if (current == root) {
			return true;
		}
	}
	return false;
}

} // namespace

void DecalComponent::OnPlayStart() {
	meshDirty_ = true;
	lastRadius_ = -1.0f;
	hasColorOverride_ = false;
	visible_ = true;
}

void DecalComponent::SetRadius(float radius) {
	if (radius <= 0.0f) {
		return;
	}
	radius_ = radius;
	meshDirty_ = true;
}

void DecalComponent::SetColorOverride(const Vector4& color) {
	hasColorOverride_ = true;
	colorOverride_ = color;
}

uint32_t DecalComponent::MaxVertexCount() const {
	// 分割数は上限まで見込んで確保する。**途中で増やせないので最大構成で取る**。
	// 円環: 円周方向 × 半径方向 × 6頂点 / 四角: 格子 × 6頂点。
	const uint32_t maxSubdivision = 128;
	const uint32_t maxRadial = 32;
	uint32_t ring = maxSubdivision * maxRadial * 6;
	uint32_t quad = maxSubdivision * maxSubdivision * 6;
	return (std::max)(ring, quad);
}

void DecalComponent::EnsureModel() {
	if (model_) {
		return;
	}
	// **TextureManagerは絶対パスしか受け付けない。** 相対のままだとPlay開始時にassertで止まる。
	std::string resolved;
	if (!texturePath_.empty()) {
		std::filesystem::path path(texturePath_);
		if (path.is_relative()) {
			path = GetProjectDataRoot() / path;
		}
		resolved = path.generic_string();
	}

	model_.reset(Model::CreateDynamic(MaxVertexCount(), resolved, static_cast<ShaderModel>(shaderModel_)));
	if (!model_) {
		return;
	}
	// 地面の模様なので裏面は要らないが、傾斜で裏返って見えるのを避けるため両面にしておく。
	model_->SetDoubleSided(true);
	// **深度は書かない。** 書くと後から描く半透明(パーティクル等)がデカールに隠される。
	model_->SetDepthWrite(false);
}

float DecalComponent::SampleGroundHeight(float x, float z) const {
	const GameObject* owner = GetOwner();
	if (!owner || !owner->GetScene()) {
		return groundY_;
	}
	Scene* scene = owner->GetScene();

	Vector3 origin = {x, owner->GetTransform().translation_.y + rayUp_, z};
	float maxDistance = rayUp_ + rayDown_;
	float nearest = maxDistance;
	bool hit = false;

	for (const std::unique_ptr<GameObject>& gameObject : scene->GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}
		// 自分の階層は地面ではない(デカールを出している本体の上に貼らない)。
		if (IsSelfOrDescendantOf(gameObject.get(), owner)) {
			continue;
		}
		// **出し元の階層も除外する。** 衝撃波は本体の子ではなくシーン直下に生成されるので、
		// 自分の階層を除くだけでは足りず、誰が出したのかを別に教えてもらう必要がある。
		if (ignoreRoot_ && IsSelfOrDescendantOf(gameObject.get(), ignoreRoot_)) {
			continue;
		}
		uint32_t layer = (std::min)(gameObject->GetLayer(), 31u);
		if ((groundLayerMask_ & (1u << layer)) == 0) {
			continue;
		}

		for (const std::unique_ptr<Component>& component : gameObject->GetComponents()) {
			ColliderComponent* collider = dynamic_cast<ColliderComponent*>(component.get());
			if (!collider || !collider->IsEnabled() || collider->IsTrigger()) {
				continue;
			}
			// 足場かどうかはコライダー単位で決める(同じオブジェクトに複数付くことがある)。
			if (staticGroundOnly_ && !IsGroundCollider(*gameObject, *collider)) {
				continue;
			}

			// **AABBの天面だけを見る。** 形ごとの厳密な交差を解かないのは、
			// デカールが乗るのはほぼ平らな床で、AABBの上面と実形状がそこでは一致するから。
			// 傾いた箱や球の上に貼ると、その分だけ実際の面より上に乗る点は割り切っている。
			AABB aabb = collider->GetWorldAABB();
			if (x < aabb.min.x || x > aabb.max.x || z < aabb.min.z || z > aabb.max.z) {
				continue;
			}
			float distance = origin.y - aabb.max.y;
			if (distance < 0.0f || distance > maxDistance) {
				continue;
			}
			hit = true;
			nearest = (std::min)(nearest, distance);
		}
	}

	if (!hit) {
		return groundY_;
	}
	return origin.y - nearest;
}

void DecalComponent::BuildQuad(std::vector<VertexData>& outVertices, const Vector3& center) const {
	int steps = std::clamp(subdivision_, 3, 128);
	float cellSize = (radius_ * 2.0f) / static_cast<float>(steps);

	for (int iz = 0; iz < steps; ++iz) {
		for (int ix = 0; ix < steps; ++ix) {
			// セルの4隅。高さは各隅で個別にレイキャストするので、床の起伏に沿う。
			float x0 = center.x - radius_ + cellSize * static_cast<float>(ix);
			float x1 = x0 + cellSize;
			float z0 = center.z - radius_ + cellSize * static_cast<float>(iz);
			float z1 = z0 + cellSize;

			float u0 = static_cast<float>(ix) / static_cast<float>(steps);
			float u1 = static_cast<float>(ix + 1) / static_cast<float>(steps);
			float v0 = static_cast<float>(iz) / static_cast<float>(steps);
			float v1 = static_cast<float>(iz + 1) / static_cast<float>(steps);

			VertexData a{{x0, SampleGroundHeight(x0, z0) + groundOffset_, z0, 1.0f}, {u0, v0}, {0.0f, 1.0f, 0.0f}};
			VertexData b{{x1, SampleGroundHeight(x1, z0) + groundOffset_, z0, 1.0f}, {u1, v0}, {0.0f, 1.0f, 0.0f}};
			VertexData c{{x1, SampleGroundHeight(x1, z1) + groundOffset_, z1, 1.0f}, {u1, v1}, {0.0f, 1.0f, 0.0f}};
			VertexData d{{x0, SampleGroundHeight(x0, z1) + groundOffset_, z1, 1.0f}, {u0, v1}, {0.0f, 1.0f, 0.0f}};

			outVertices.push_back(a);
			outVertices.push_back(b);
			outVertices.push_back(c);
			outVertices.push_back(a);
			outVertices.push_back(c);
			outVertices.push_back(d);
		}
	}
}

void DecalComponent::BuildRing(std::vector<VertexData>& outVertices, const Vector3& center) const {
	int around = std::clamp(subdivision_, 3, 128);
	int radial = std::clamp(radialSteps_, 1, 32);
	float inner = radius_ * std::clamp(innerRatio_, 0.0f, 0.99f);
	float twoPi = std::numbers::pi_v<float> * 2.0f;

	auto vertexAt = [&](int angleIndex, int radialIndex) {
		float angle = twoPi * static_cast<float>(angleIndex) / static_cast<float>(around);
		float t = static_cast<float>(radialIndex) / static_cast<float>(radial);
		float r = inner + (radius_ - inner) * t;
		float x = center.x + std::cos(angle) * r;
		float z = center.z + std::sin(angle) * r;
		// uは円周、vは内周0→外周1。**既存のリング用シェーダーと同じ規約**にしてある。
		float u = static_cast<float>(angleIndex) / static_cast<float>(around);
		return VertexData{{x, SampleGroundHeight(x, z) + groundOffset_, z, 1.0f}, {u, t}, {0.0f, 1.0f, 0.0f}};
	};

	for (int a = 0; a < around; ++a) {
		for (int r = 0; r < radial; ++r) {
			VertexData v00 = vertexAt(a, r);
			VertexData v10 = vertexAt(a + 1, r);
			VertexData v11 = vertexAt(a + 1, r + 1);
			VertexData v01 = vertexAt(a, r + 1);

			outVertices.push_back(v00);
			outVertices.push_back(v10);
			outVertices.push_back(v11);
			outVertices.push_back(v00);
			outVertices.push_back(v11);
			outVertices.push_back(v01);
		}
	}
}

void DecalComponent::RebuildMesh() {
	GameObject* owner = GetOwner();
	if (!owner || !model_) {
		return;
	}

	owner->UpdateWorldTransformSelfAndAncestors();
	Vector3 center = owner->GetTransform().GetWorldPosition();

	vertices_.clear();
	if (shape_ == 0) {
		BuildQuad(vertices_, center);
	} else {
		BuildRing(vertices_, center);
	}
	model_->UpdateDynamicVertices(vertices_);

	lastCenter_ = center;
	lastRadius_ = radius_;
	meshDirty_ = false;
}

void DecalComponent::Update() {
	// **モデルの生成(=テクスチャ読み込み)は描画パスの外で行う。**
	EnsureModel();
	if (!model_ || !visible_) {
		return;
	}

	if (!transformReady_) {
		identityTransform_.Initialize();
		transformReady_ = true;
	}

	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}

	if (followOwner_) {
		owner->UpdateWorldTransformSelfAndAncestors();
		Vector3 center = owner->GetTransform().GetWorldPosition();
		Vector3 moved = center - lastCenter_;
		// **動いていなければ組み直さない。** 頂点ごとにレイキャストするので、
		// 毎フレーム無条件に作り直すと分割数がそのまま負荷になる。
		if (moved.x * moved.x + moved.y * moved.y + moved.z * moved.z > 1.0e-6f) {
			meshDirty_ = true;
		}
	}
	if (std::fabs(radius_ - lastRadius_) > 1.0e-4f) {
		meshDirty_ = true;
	}

	if (meshDirty_) {
		RebuildMesh();
	}
}

void DecalComponent::Draw() {
	if (!model_ || !camera_ || !visible_ || vertices_.empty()) {
		return;
	}

	model_->SetBlendMode(static_cast<BlendMode>(blendMode_));
	if (hasColorOverride_) {
		model_->SetColor(colorOverride_);
	}

	// 頂点はワールド座標で作ってあるので、ワールド行列は単位のまま描く。
	identityTransform_.UpdateMatrix(*camera_);
	model_->Draw(identityTransform_, *camera_);
}

} // namespace KujataEngine
