#include "ShapeUtil.h"

namespace KujakuEngine {
namespace ShapeUtil {

bool IsCollision(const Sphere& sphere, const Plane& plane) {
	float k = std::abs(Vector3::Dot(plane.normal, sphere.center) - plane.distance);
	return (k <= sphere.radius);
}

bool IsCollision(const Segment& segment, const Plane& plane) {
	float dot = Vector3::Dot(plane.normal, segment.diff);

	if (dot == 0.0f) {
		return false;
	}

	float t = (plane.distance - Vector3::Dot(segment.origin, plane.normal)) / dot;

	return (t >= 0.0f && t <= 1.0f);
}

bool IsCollision(const Segment& segment, const Triangle& triangle) {
	Vector3 normal = Vector3::Normalize(Vector3::Cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));

	float dot = Vector3::Dot(normal, segment.diff);

	if (dot == 0.0f) {
		return false;
	}

	float d = Vector3::Dot(normal, triangle.vertices[0]);
	float t = (d - Vector3::Dot(segment.origin, normal)) / dot;

	Vector3 p = segment.origin + segment.diff * t;

	Vector3 v01 = triangle.vertices[1] - triangle.vertices[0];
	Vector3 v12 = triangle.vertices[2] - triangle.vertices[1];
	Vector3 v20 = triangle.vertices[0] - triangle.vertices[2];
	Vector3 v0p = p - triangle.vertices[0];
	Vector3 v1p = p - triangle.vertices[1];
	Vector3 v2p = p - triangle.vertices[2];
	Vector3 cross01 = Vector3::Cross(v01, v1p);
	Vector3 cross12 = Vector3::Cross(v12, v2p);
	Vector3 cross20 = Vector3::Cross(v20, v0p);

	bool resultA = (Vector3::Dot(cross01, normal) >= 0.0f && Vector3::Dot(cross12, normal) >= 0.0f && Vector3::Dot(cross20, normal) >= 0.0f);
	bool resultB = (t >= 0.0f && t <= 1.0f);

	return (resultA && resultB);
}

bool IsCollision(const AABB& aabb1, const AABB& aabb2) {
	bool resultX = (aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x);
	bool resultY = (aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y);
	bool resultZ = (aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);

	return (resultX && resultY && resultZ);
}

bool IsCollision(const AABB& aabb, const Sphere& sphere) {
	// 最近接点を求める
	Vector3 closestPoint{
	    std::clamp(sphere.center.x, aabb.min.x, aabb.max.x),
	    std::clamp(sphere.center.y, aabb.min.y, aabb.max.y),
	    std::clamp(sphere.center.z, aabb.min.z, aabb.max.z),
	};

	// 最近接点と球の中心との距離を求める
	float distance = Vector3::Length(closestPoint - sphere.center);

	// 距離が半径よりも小さければ衝突
	return (distance <= sphere.radius);
}

bool IsCollision(const AABB& aabb, const Segment& segment) {
	float txMin = (aabb.min.x - segment.origin.x) / segment.diff.x;
	float txMax = (aabb.max.x - segment.origin.x) / segment.diff.x;
	float tyMin = (aabb.min.y - segment.origin.y) / segment.diff.y;
	float tyMax = (aabb.max.y - segment.origin.y) / segment.diff.y;
	float tzMin = (aabb.min.z - segment.origin.z) / segment.diff.z;
	float tzMax = (aabb.max.z - segment.origin.z) / segment.diff.z;

	float tNearX = (std::min)(txMin, txMax);
	float tNearY = (std::min)(tyMin, tyMax);
	float tNearZ = (std::min)(tzMin, tzMax);
	float tFarX = (std::max)(txMin, txMax);
	float tFarY = (std::max)(tyMin, tyMax);
	float tFarZ = (std::max)(tzMin, tzMax);

	// AABBとの衝突点(貫通点)のtが小さい方
	float tmin = (std::max)((std::max)(tNearX, tNearY), tNearZ);
	// AABBとの衝突点(貫通点)のtが大きい方
	float tmax = (std::min)((std::min)(tFarX, tFarY), tFarZ);

	// Segment
	if (tmin <= tmax) {
		if (tmax >= 0.0f && tmin <= 1.0f) {
			return true;
		}
	}
	return false;
}

bool IsCollision(const AABB& aabb, const Line& line) {
	float txMin = (aabb.min.x - line.origin.x) / line.diff.x;
	float txMax = (aabb.max.x - line.origin.x) / line.diff.x;
	float tyMin = (aabb.min.y - line.origin.y) / line.diff.y;
	float tyMax = (aabb.max.y - line.origin.y) / line.diff.y;
	float tzMin = (aabb.min.z - line.origin.z) / line.diff.z;
	float tzMax = (aabb.max.z - line.origin.z) / line.diff.z;

	float tNearX = (std::min)(txMin, txMax);
	float tNearY = (std::min)(tyMin, tyMax);
	float tNearZ = (std::min)(tzMin, tzMax);
	float tFarX = (std::max)(txMin, txMax);
	float tFarY = (std::max)(tyMin, tyMax);
	float tFarZ = (std::max)(tzMin, tzMax);

	// AABBとの衝突点(貫通点)のtが小さい方
	float tmin = (std::max)((std::max)(tNearX, tNearY), tNearZ);
	// AABBとの衝突点(貫通点)のtが大きい方
	float tmax = (std::min)((std::min)(tFarX, tFarY), tFarZ);

	// Line
	if (tmin <= tmax) {
		return true;
	}

	return false;
}

bool IsCollision(const AABB& aabb, const Ray& ray) {
	float txMin = (aabb.min.x - ray.origin.x) / ray.diff.x;
	float txMax = (aabb.max.x - ray.origin.x) / ray.diff.x;
	float tyMin = (aabb.min.y - ray.origin.y) / ray.diff.y;
	float tyMax = (aabb.max.y - ray.origin.y) / ray.diff.y;
	float tzMin = (aabb.min.z - ray.origin.z) / ray.diff.z;
	float tzMax = (aabb.max.z - ray.origin.z) / ray.diff.z;

	float tNearX = (std::min)(txMin, txMax);
	float tNearY = (std::min)(tyMin, tyMax);
	float tNearZ = (std::min)(tzMin, tzMax);
	float tFarX = (std::max)(txMin, txMax);
	float tFarY = (std::max)(tyMin, tyMax);
	float tFarZ = (std::max)(tzMin, tzMax);

	// AABBとの衝突点(貫通点)のtが小さい方
	float tmin = (std::max)((std::max)(tNearX, tNearY), tNearZ);
	// AABBとの衝突点(貫通点)のtが大きい方
	float tmax = (std::min)((std::min)(tFarX, tFarY), tFarZ);

	// Ray
	if (tmin <= tmax) {
		if (tmax >= 0.0f) {
			return true;
		}
	}

	return false;
}

bool IsCollision(const OBB& obb, const Sphere& sphere) {
	Matrix4x4 obbWorldMatrix = obb.GetWorldMatrix();
	Matrix4x4 obbWorldMatrixInverse = Matrix4x4::Inverse(obbWorldMatrix);
	Vector3 centerInOBBLocalSpace = Vector3::Transform(sphere.center, obbWorldMatrixInverse);

	AABB aabbOBBLocal{.min = -obb.size, .max = obb.size};
	Sphere sphereOBBLocal{centerInOBBLocalSpace, sphere.radius};
	// ローカル空間で衝突判定
	return IsCollision(aabbOBBLocal, sphereOBBLocal);
}

bool IsCollision(const OBB& obb, const Segment& segment) {
	Matrix4x4 obbWorldMatrix = obb.GetWorldMatrix();
	Matrix4x4 obbWorldMatrixInverse = Matrix4x4::Inverse(obbWorldMatrix);

	Vector3 localOrigin = Vector3::Transform(segment.origin, obbWorldMatrixInverse);
	Vector3 localEnd = Vector3::Transform(segment.origin + segment.diff, obbWorldMatrixInverse);

	AABB localAABB{
	    -obb.size,
	    obb.size,
	};

	Segment localSegment;
	localSegment.origin = localOrigin;
	localSegment.diff = localEnd - localOrigin;
	return IsCollision(localAABB, localSegment);
}

bool IsCollision(const OBB& obb, const Line& line) {
	Matrix4x4 obbWorldMatrix = obb.GetWorldMatrix();
	Matrix4x4 obbWorldMatrixInverse = Matrix4x4::Inverse(obbWorldMatrix);

	Vector3 localOrigin = Vector3::Transform(line.origin, obbWorldMatrixInverse);
	Vector3 localEnd = Vector3::Transform(line.origin + line.diff, obbWorldMatrixInverse);

	AABB localAABB{
	    -obb.size,
	    obb.size,
	};

	Line localLine;
	localLine.origin = localOrigin;
	localLine.diff = localEnd - localOrigin;
	return IsCollision(localAABB, localLine);
}

bool IsCollision(const OBB& obb, const Ray& ray) {
	Matrix4x4 obbWorldMatrix = obb.GetWorldMatrix();
	Matrix4x4 obbWorldMatrixInverse = Matrix4x4::Inverse(obbWorldMatrix);

	Vector3 localOrigin = Vector3::Transform(ray.origin, obbWorldMatrixInverse);
	Vector3 localEnd = Vector3::Transform(ray.origin + ray.diff, obbWorldMatrixInverse);

	AABB localAABB{
	    -obb.size,
	    obb.size,
	};

	Ray localRay;
	localRay.origin = localOrigin;
	localRay.diff = localEnd - localOrigin;
	return IsCollision(localAABB, localRay);
}

bool IsOverlappingOnAxis(const OBB& obb1, const OBB& obb2, const Vector3& axis) {
	Vector3 L = Vector3::Normalize(axis);

	// 全てワールド座標からの Dot で統一する
	auto GetProjection = [&](const OBB& obb) {
		float center = Vector3::Dot(obb.center, L);
		float extent =
		    std::abs(Vector3::Dot(obb.orientations[0], L)) * obb.size.x + std::abs(Vector3::Dot(obb.orientations[1], L)) * obb.size.y + std::abs(Vector3::Dot(obb.orientations[2], L)) * obb.size.z;
		return std::make_pair(center - extent, center + extent);
	};

	auto [min1, max1] = GetProjection(obb1);
	auto [min2, max2] = GetProjection(obb2);

	float sumSpan = (max1 - min1) + (max2 - min2);
	float longSpan = (std::max)(max1, max2) - (std::min)(min1, min2);

	return sumSpan >= longSpan;
}

bool IsCollision(const OBB& obb1, const OBB& obb2) {
	if (!IsOverlappingOnAxis(obb1, obb2, obb1.orientations[0])) {
		return false;
	}
	if (!IsOverlappingOnAxis(obb1, obb2, obb1.orientations[1])) {
		return false;
	}
	if (!IsOverlappingOnAxis(obb1, obb2, obb1.orientations[2])) {
		return false;
	}

	if (!IsOverlappingOnAxis(obb1, obb2, obb2.orientations[0])) {
		return false;
	}
	if (!IsOverlappingOnAxis(obb1, obb2, obb2.orientations[1])) {
		return false;
	}
	if (!IsOverlappingOnAxis(obb1, obb2, obb2.orientations[2])) {
		return false;
	}

	Vector3 c;

	c = Vector3::Cross(obb1.orientations[0], obb2.orientations[0]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[0], obb2.orientations[1]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[0], obb2.orientations[2]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[1], obb2.orientations[0]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[1], obb2.orientations[1]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[1], obb2.orientations[2]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[2], obb2.orientations[0]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[2], obb2.orientations[1]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Vector3::Cross(obb1.orientations[2], obb2.orientations[2]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	return true;
}

Vector3 Reflect(const Vector3& input, const Vector3& normal) { return input - normal * (2.0f * Vector3::Dot(input, normal)); }

} // namespace ShapeUtil

} // namespace KujakuEngine


