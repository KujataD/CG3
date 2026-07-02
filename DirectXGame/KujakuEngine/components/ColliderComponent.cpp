#include "ColliderComponent.h"
#include "../Editor/InspectorUI.h"
#include "../math/MathUtil.h"
#include "../scene/GameObject.h"
#include "../shapes/ShapeUtil.h"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace KujakuEngine {

namespace {

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

bool IntersectsByShape(const ColliderComponent& a, const ColliderComponent& b) {
	if (a.GetShapeType() == ColliderShapeType::Sphere && b.GetShapeType() == ColliderShapeType::Sphere) {
		return ShapeUtil::IsCollision(a.GetWorldSphere(), b.GetWorldSphere());
	}

	if (a.GetShapeType() == ColliderShapeType::Box && b.GetShapeType() == ColliderShapeType::Box) {
		return ShapeUtil::IsCollision(a.GetWorldAABB(), b.GetWorldAABB());
	}

	if (a.GetShapeType() == ColliderShapeType::Box && b.GetShapeType() == ColliderShapeType::Sphere) {
		return ShapeUtil::IsCollision(a.GetWorldAABB(), b.GetWorldSphere());
	}

	if (a.GetShapeType() == ColliderShapeType::Sphere && b.GetShapeType() == ColliderShapeType::Box) {
		return ShapeUtil::IsCollision(b.GetWorldAABB(), a.GetWorldSphere());
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
	AABB aabb = GetWorldAABB();
	Vector3 center = (aabb.min + aabb.max) * 0.5f;
	float radius = Length(aabb.max - center);
	return {center, radius};
}

AABB BoxColliderComponent::GetWorldAABB() const {
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
