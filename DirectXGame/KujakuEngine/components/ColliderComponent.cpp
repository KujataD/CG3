#include "ColliderComponent.h"
#include "../runtime/InspectorUI.h"
#include "../math/MathUtil.h"
#include "../scene/GameObject.h"
#include "../shapes/ShapeUtil.h"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace KujakuEngine {

namespace {

constexpr float kWorldRotationEpsilon = 0.0001f;

uint64_t GenerateColliderRuntimeId() {
	static std::atomic<uint64_t> counter = 0;
	return ++counter;
}

Vector3 ReadVector3(const nlohmann::json& json, const char* key, const Vector3& defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	if (!json.at(key).is_array()) {
		return defaultValue;
	}
	if (json.at(key).size() < 3) {
		return defaultValue;
	}

	Vector3 value = defaultValue;
	if (json.at(key).at(0).is_number()) {
		value.x = json.at(key).at(0).get<float>();
	}
	if (json.at(key).at(1).is_number()) {
		value.y = json.at(key).at(1).get<float>();
	}
	if (json.at(key).at(2).is_number()) {
		value.z = json.at(key).at(2).get<float>();
	}
	return value;
}

bool ReadBool(const nlohmann::json& json, const char* key, bool defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	if (!json.at(key).is_boolean()) {
		return defaultValue;
	}
	return json.at(key).get<bool>();
}

uint32_t ReadUint32(const nlohmann::json& json, const char* key, uint32_t defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	if (!json.at(key).is_number_unsigned() && !json.at(key).is_number_integer()) {
		return defaultValue;
	}

	int64_t value = json.at(key).get<int64_t>();
	if (value < 0) {
		return defaultValue;
	}
	return static_cast<uint32_t>(value);
}

float ReadFloat(const nlohmann::json& json, const char* key, float defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	if (!json.at(key).is_number()) {
		return defaultValue;
	}
	return json.at(key).get<float>();
}

Vector3 AbsoluteVector(const Vector3& value) {
	return {std::fabs(value.x), std::fabs(value.y), std::fabs(value.z)};
}

Vector3 MaxVector(const Vector3& a, const Vector3& b) {
	return {
	    (std::max)(a.x, b.x),
	    (std::max)(a.y, b.y),
	    (std::max)(a.z, b.z),
	};
}

bool IsNearlyEqual(float a, float b) {
	return std::fabs(a - b) <= kWorldRotationEpsilon;
}

bool IsNearlyEqual(const Vector3& a, const Vector3& b) {
	if (!IsNearlyEqual(a.x, b.x)) {
		return false;
	}
	if (!IsNearlyEqual(a.y, b.y)) {
		return false;
	}
	if (!IsNearlyEqual(a.z, b.z)) {
		return false;
	}
	return true;
}

Vector3 GetWorldAxis(const Matrix4x4& matrix, uint32_t axisIndex) {
	Vector3 axis = {matrix.m[axisIndex][0], matrix.m[axisIndex][1], matrix.m[axisIndex][2]};
	float length = Length(axis);
	if (length <= kWorldRotationEpsilon) {
		if (axisIndex == 0) {
			return {1.0f, 0.0f, 0.0f};
		}
		if (axisIndex == 1) {
			return {0.0f, 1.0f, 0.0f};
		}
		return {0.0f, 0.0f, 1.0f};
	}
	return axis / length;
}

float GetWorldAxisScale(const Matrix4x4& matrix, uint32_t axisIndex) {
	Vector3 axis = {matrix.m[axisIndex][0], matrix.m[axisIndex][1], matrix.m[axisIndex][2]};
	return Length(axis);
}

bool HasWorldRotation(const Matrix4x4& matrix) {
	if (!IsNearlyEqual(GetWorldAxis(matrix, 0), {1.0f, 0.0f, 0.0f})) {
		return true;
	}
	if (!IsNearlyEqual(GetWorldAxis(matrix, 1), {0.0f, 1.0f, 0.0f})) {
		return true;
	}
	if (!IsNearlyEqual(GetWorldAxis(matrix, 2), {0.0f, 0.0f, 1.0f})) {
		return true;
	}
	return false;
}

const BoxColliderComponent* AsBoxCollider(const ColliderComponent& collider) {
	return dynamic_cast<const BoxColliderComponent*>(&collider);
}

bool IsCollisionBoxAndBox(const ColliderComponent& a, const ColliderComponent& b) {
	const BoxColliderComponent* boxA = AsBoxCollider(a);
	const BoxColliderComponent* boxB = AsBoxCollider(b);
	if (!boxA || !boxB) {
		return ShapeUtil::IsCollision(a.GetWorldAABB(), b.GetWorldAABB());
	}

	if (boxA->UsesWorldOBB() || boxB->UsesWorldOBB()) {
		return ShapeUtil::IsCollision(boxA->GetWorldOBB(), boxB->GetWorldOBB());
	}

	return ShapeUtil::IsCollision(boxA->GetWorldAABB(), boxB->GetWorldAABB());
}

bool IsCollisionBoxAndSphere(const ColliderComponent& boxCollider, const ColliderComponent& sphereCollider) {
	const BoxColliderComponent* box = AsBoxCollider(boxCollider);
	if (!box) {
		return ShapeUtil::IsCollision(boxCollider.GetWorldAABB(), sphereCollider.GetWorldSphere());
	}

	if (box->UsesWorldOBB()) {
		return ShapeUtil::IsCollision(box->GetWorldOBB(), sphereCollider.GetWorldSphere());
	}

	return ShapeUtil::IsCollision(box->GetWorldAABB(), sphereCollider.GetWorldSphere());
}

// 任意ColliderをワールドOBBへ変換する(Box以外はAABBから軸並行OBBを作る)。
OBB MakeWorldOBB(const ColliderComponent& collider) {
	if (const BoxColliderComponent* box = AsBoxCollider(collider)) {
		return box->GetWorldOBB();
	}
	AABB aabb = collider.GetWorldAABB();
	OBB obb{};
	obb.center = (aabb.min + aabb.max) * 0.5f;
	obb.orientations[0] = {1.0f, 0.0f, 0.0f};
	obb.orientations[1] = {0.0f, 1.0f, 0.0f};
	obb.orientations[2] = {0.0f, 0.0f, 1.0f};
	obb.size = (aabb.max - aabb.min) * 0.5f;
	return obb;
}

// a→b への接触情報を計算する(normal は a から b へ向かう分離方向)。
bool ComputeContactByShape(const ColliderComponent& a, const ColliderComponent& b, Contact& out) {
	const bool aSphere = a.GetShapeType() == ColliderShapeType::Sphere;
	const bool bSphere = b.GetShapeType() == ColliderShapeType::Sphere;

	if (aSphere && bSphere) {
		return ShapeUtil::ComputeContact(a.GetWorldSphere(), b.GetWorldSphere(), out);
	}
	if (!aSphere && !bSphere) {
		return ShapeUtil::ComputeContact(MakeWorldOBB(a), MakeWorldOBB(b), out);
	}
	if (aSphere && !bSphere) {
		// 球a→箱b: normal は sphere→obb = a→b
		return ShapeUtil::ComputeContact(a.GetWorldSphere(), MakeWorldOBB(b), out);
	}
	// 箱a→球b: sphere(b)→obb(a) を求めると normal は b→a なので反転する
	if (ShapeUtil::ComputeContact(b.GetWorldSphere(), MakeWorldOBB(a), out)) {
		out.normal = out.normal * -1.0f;
		return true;
	}
	return false;
}

bool IntersectsByShape(const ColliderComponent& a, const ColliderComponent& b) {
	if (a.GetShapeType() == ColliderShapeType::Sphere && b.GetShapeType() == ColliderShapeType::Sphere) {
		return ShapeUtil::IsCollision(a.GetWorldSphere(), b.GetWorldSphere());
	}

	if (a.GetShapeType() == ColliderShapeType::Box && b.GetShapeType() == ColliderShapeType::Box) {
		return IsCollisionBoxAndBox(a, b);
	}

	if (a.GetShapeType() == ColliderShapeType::Box && b.GetShapeType() == ColliderShapeType::Sphere) {
		return IsCollisionBoxAndSphere(a, b);
	}

	if (a.GetShapeType() == ColliderShapeType::Sphere && b.GetShapeType() == ColliderShapeType::Box) {
		return IsCollisionBoxAndSphere(b, a);
	}

	return false;
}

} // namespace

ColliderComponent::ColliderComponent() : runtimeId_(GenerateColliderRuntimeId()) {}

uint32_t ColliderComponent::GetLayerMaskBit() const {
	GameObject* owner = GetOwner();
	if (!owner) {
		return 1u;
	}

	uint32_t layer = owner->GetLayer();
	if (layer > 31) {
		layer = 31;
	}
	return 1u << layer;
}

bool ColliderComponent::CanCollideWith(const ColliderComponent& other) const {
	uint32_t selfLayerBit = GetLayerMaskBit();
	uint32_t otherLayerBit = other.GetLayerMaskBit();

	if ((collisionMask_ & otherLayerBit) == 0) {
		return false;
	}
	if ((other.collisionMask_ & selfLayerBit) == 0) {
		return false;
	}

	return true;
}

Vector3 ColliderComponent::GetWorldCenter() const {
	GameObject* owner = GetOwner();
	if (!owner) {
		return center_;
	}

	const_cast<GameObject*>(owner)->UpdateWorldTransformSelfAndAncestors();
	return Transform(center_, owner->GetTransform().matWorld_);
}

Sphere ColliderComponent::GetWorldSphere() const {
	return {GetWorldCenter(), 0.0f};
}

AABB ColliderComponent::GetWorldAABB() const {
	Vector3 center = GetWorldCenter();
	return {center, center};
}

bool ColliderComponent::Intersects(const ColliderComponent& other) const {
	if (!CanCollideWith(other)) {
		return false;
	}

	return IntersectsByShape(*this, other);
}

bool ColliderComponent::ComputeContact(const ColliderComponent& other, Contact& out) const {
	return ComputeContactByShape(*this, other, out);
}

void ColliderComponent::DrawInspector() {
	bool isTrigger = isTrigger_;
	if (InspectorUI::Checkbox("Is Trigger", &isTrigger)) {
		isTrigger_ = isTrigger;
	}

	InspectorUI::DragFloat3("Center", &center_.x, 0.01f);

	int collisionMask = -1;
	if (collisionMask_ != 0xffffffff) {
		collisionMask = static_cast<int>(collisionMask_);
	}
	if (InspectorUI::DragInt("Collision Mask", &collisionMask, 1.0f, -1, 0x7fffffff)) {
		if (collisionMask < 0) {
			collisionMask_ = 0xffffffff;
		} else {
			collisionMask_ = static_cast<uint32_t>(collisionMask);
		}
	}

	DrawShapeInspector();
}

void ColliderComponent::WriteJson(nlohmann::json& json) const {
	json["isTrigger"] = isTrigger_;
	json["center"] = {center_.x, center_.y, center_.z};
	json["collisionMask"] = collisionMask_;
	WriteShapeJson(json);
}

void ColliderComponent::ReadJson(const nlohmann::json& json) {
	isTrigger_ = ReadBool(json, "isTrigger", isTrigger_);
	center_ = ReadVector3(json, "center", center_);
	collisionMask_ = ReadUint32(json, "collisionMask", collisionMask_);
	ReadShapeJson(json);
}

void SphereColliderComponent::SetRadius(float radius) {
	if (radius < 0.0f) {
		radius = 0.0f;
	}
	radius_ = radius;
}

Sphere SphereColliderComponent::GetWorldSphere() const {
	float maxScale = 1.0f;
	GameObject* owner = GetOwner();
	if (owner) {
		Vector3 scale = AbsoluteVector(owner->GetTransform().scale_);
		maxScale = (std::max)(scale.x, (std::max)(scale.y, scale.z));
	}

	return {GetWorldCenter(), radius_ * maxScale};
}

AABB SphereColliderComponent::GetWorldAABB() const {
	Sphere sphere = GetWorldSphere();
	Vector3 radiusVector = {sphere.radius, sphere.radius, sphere.radius};
	return {sphere.center - radiusVector, sphere.center + radiusVector};
}

void SphereColliderComponent::DrawShapeInspector() {
	InspectorUI::DragFloat("Radius", &radius_, 0.01f, 0.0f, 0.0f);
	if (radius_ < 0.0f) {
		radius_ = 0.0f;
	}
}

void SphereColliderComponent::WriteShapeJson(nlohmann::json& json) const {
	json["radius"] = radius_;
}

void SphereColliderComponent::ReadShapeJson(const nlohmann::json& json) {
	SetRadius(ReadFloat(json, "radius", radius_));
}

void BoxColliderComponent::SetSize(const Vector3& size) {
	size_ = MaxVector(size, {0.0f, 0.0f, 0.0f});
}

Sphere BoxColliderComponent::GetWorldSphere() const {
	OBB obb = GetWorldOBB();
	float radius = Length(obb.size);
	Vector3 center = obb.center;
	return {center, radius};
}

AABB BoxColliderComponent::GetWorldAABB() const {
	if (UsesWorldOBB()) {
		OBB obb = GetWorldOBB();
		Vector3 corners[8] = {
		    obb.center - obb.orientations[0] * obb.size.x - obb.orientations[1] * obb.size.y - obb.orientations[2] * obb.size.z,
		    obb.center + obb.orientations[0] * obb.size.x - obb.orientations[1] * obb.size.y - obb.orientations[2] * obb.size.z,
		    obb.center + obb.orientations[0] * obb.size.x + obb.orientations[1] * obb.size.y - obb.orientations[2] * obb.size.z,
		    obb.center - obb.orientations[0] * obb.size.x + obb.orientations[1] * obb.size.y - obb.orientations[2] * obb.size.z,
		    obb.center - obb.orientations[0] * obb.size.x - obb.orientations[1] * obb.size.y + obb.orientations[2] * obb.size.z,
		    obb.center + obb.orientations[0] * obb.size.x - obb.orientations[1] * obb.size.y + obb.orientations[2] * obb.size.z,
		    obb.center + obb.orientations[0] * obb.size.x + obb.orientations[1] * obb.size.y + obb.orientations[2] * obb.size.z,
		    obb.center - obb.orientations[0] * obb.size.x + obb.orientations[1] * obb.size.y + obb.orientations[2] * obb.size.z,
		};

		AABB aabb{corners[0], corners[0]};
		for (const Vector3& corner : corners) {
			aabb.min.x = (std::min)(aabb.min.x, corner.x);
			aabb.min.y = (std::min)(aabb.min.y, corner.y);
			aabb.min.z = (std::min)(aabb.min.z, corner.z);
			aabb.max.x = (std::max)(aabb.max.x, corner.x);
			aabb.max.y = (std::max)(aabb.max.y, corner.y);
			aabb.max.z = (std::max)(aabb.max.z, corner.z);
		}
		return aabb;
	}

	Vector3 center = GetWorldCenter();
	Vector3 size = size_;
	GameObject* owner = GetOwner();
	if (owner) {
		Vector3 scale = AbsoluteVector(owner->GetTransform().scale_);
		size.x *= scale.x;
		size.y *= scale.y;
		size.z *= scale.z;
	}

	Vector3 halfSize = size * 0.5f;
	AABB aabb{center - halfSize, center + halfSize};
	aabb.SwapMinMax();
	return aabb;
}

bool BoxColliderComponent::UsesWorldOBB() const {
	GameObject* owner = GetOwner();
	if (!owner) {
		return false;
	}

	const_cast<GameObject*>(owner)->UpdateWorldTransformSelfAndAncestors();
	return HasWorldRotation(owner->GetTransform().matWorld_);
}

OBB BoxColliderComponent::GetWorldOBB() const {
	OBB obb{};
	obb.center = GetWorldCenter();
	obb.orientations[0] = {1.0f, 0.0f, 0.0f};
	obb.orientations[1] = {0.0f, 1.0f, 0.0f};
	obb.orientations[2] = {0.0f, 0.0f, 1.0f};
	obb.size = size_ * 0.5f;

	GameObject* owner = GetOwner();
	if (!owner) {
		return obb;
	}

	const_cast<GameObject*>(owner)->UpdateWorldTransformSelfAndAncestors();
	const Matrix4x4& matrix = owner->GetTransform().matWorld_;
	obb.orientations[0] = GetWorldAxis(matrix, 0);
	obb.orientations[1] = GetWorldAxis(matrix, 1);
	obb.orientations[2] = GetWorldAxis(matrix, 2);
	obb.size.x *= GetWorldAxisScale(matrix, 0);
	obb.size.y *= GetWorldAxisScale(matrix, 1);
	obb.size.z *= GetWorldAxisScale(matrix, 2);
	return obb;
}

void BoxColliderComponent::DrawShapeInspector() {
	InspectorUI::DragFloat3("Size", &size_.x, 0.01f, 0.0f, 0.0f);
	SetSize(size_);
}

void BoxColliderComponent::WriteShapeJson(nlohmann::json& json) const {
	json["size"] = {size_.x, size_.y, size_.z};
}

void BoxColliderComponent::ReadShapeJson(const nlohmann::json& json) {
	SetSize(ReadVector3(json, "size", size_));
}

} // namespace KujakuEngine
