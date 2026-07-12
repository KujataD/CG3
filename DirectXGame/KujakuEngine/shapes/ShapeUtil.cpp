#include "ShapeUtil.h"
#include <algorithm>
#include <cassert>

namespace KujakuEngine {
namespace ShapeUtil {

bool IsCollision(const Sphere& sphere, const Plane& plane) {
	float k = std::abs(Dot(plane.normal, sphere.center) - plane.distance);
	return (k <= sphere.radius);
}

bool IsCollision(const Segment& segment, const Plane& plane) {
	float dot = Dot(plane.normal, segment.diff);

	if (dot == 0.0f) {
		return false;
	}

	float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;

	return (t >= 0.0f && t <= 1.0f);
}

bool IsCollision(const Segment& segment, const Triangle& triangle) {
	Vector3 normal = Normalize(Cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));

	float dot = Dot(normal, segment.diff);

	if (dot == 0.0f) {
		return false;
	}

	float d = Dot(normal, triangle.vertices[0]);
	float t = (d - Dot(segment.origin, normal)) / dot;

	Vector3 p = segment.origin + segment.diff * t;

	Vector3 v01 = triangle.vertices[1] - triangle.vertices[0];
	Vector3 v12 = triangle.vertices[2] - triangle.vertices[1];
	Vector3 v20 = triangle.vertices[0] - triangle.vertices[2];
	Vector3 v0p = p - triangle.vertices[0];
	Vector3 v1p = p - triangle.vertices[1];
	Vector3 v2p = p - triangle.vertices[2];
	Vector3 cross01 = Cross(v01, v1p);
	Vector3 cross12 = Cross(v12, v2p);
	Vector3 cross20 = Cross(v20, v0p);

	bool resultA = (Dot(cross01, normal) >= 0.0f && Dot(cross12, normal) >= 0.0f && Dot(cross20, normal) >= 0.0f);
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
	float distance = Length(closestPoint - sphere.center);

	// 距離が半径よりも小さければ衝突
	return (distance <= sphere.radius);
}

bool IsCollision(const AABB& aabb, const Vector3& point) {
	bool resultX = (aabb.min.x <= point.x && aabb.max.x >= point.x);
	bool resultY = (aabb.min.y <= point.y && aabb.max.y >= point.y);
	bool resultZ = (aabb.min.z <= point.z && aabb.max.z >= point.z);

	return (resultX && resultY && resultZ);
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
	Matrix4x4 obbWorldMatrixInverse = Inverse(obbWorldMatrix);
	Vector3 centerInOBBLocalSpace = Transform(sphere.center, obbWorldMatrixInverse);

	AABB aabbOBBLocal{.min = -obb.size, .max = obb.size};
	Sphere sphereOBBLocal{centerInOBBLocalSpace, sphere.radius};
	// ローカル空間で衝突判定
	return IsCollision(aabbOBBLocal, sphereOBBLocal);
}

bool IsCollision(const OBB& obb, const Segment& segment) {
	Matrix4x4 obbWorldMatrix = obb.GetWorldMatrix();
	Matrix4x4 obbWorldMatrixInverse = Inverse(obbWorldMatrix);

	Vector3 localOrigin = Transform(segment.origin, obbWorldMatrixInverse);
	Vector3 localEnd = Transform(segment.origin + segment.diff, obbWorldMatrixInverse);

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
	Matrix4x4 obbWorldMatrixInverse = Inverse(obbWorldMatrix);

	Vector3 localOrigin = Transform(line.origin, obbWorldMatrixInverse);
	Vector3 localEnd = Transform(line.origin + line.diff, obbWorldMatrixInverse);

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
	Matrix4x4 obbWorldMatrixInverse = Inverse(obbWorldMatrix);

	Vector3 localOrigin = Transform(ray.origin, obbWorldMatrixInverse);
	Vector3 localEnd = Transform(ray.origin + ray.diff, obbWorldMatrixInverse);

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
	Vector3 L = Normalize(axis);

	// 全てワールド座標からの Dot で統一する
	auto GetProjection = [&](const OBB& obb) {
		float center = Dot(obb.center, L);
		float extent = std::abs(Dot(obb.orientations[0], L)) * obb.size.x + std::abs(Dot(obb.orientations[1], L)) * obb.size.y + std::abs(Dot(obb.orientations[2], L)) * obb.size.z;
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

	c = Cross(obb1.orientations[0], obb2.orientations[0]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[0], obb2.orientations[1]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[0], obb2.orientations[2]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[1], obb2.orientations[0]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[1], obb2.orientations[1]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[1], obb2.orientations[2]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[2], obb2.orientations[0]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[2], obb2.orientations[1]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	c = Cross(obb1.orientations[2], obb2.orientations[2]);
	if (!IsOverlappingOnAxis(obb1, obb2, c)) {
		return false;
	}

	return true;
}

bool IsCollision(const Sphere& a, const Sphere& b) { return powf(b.center.x - a.center.x, 2) + powf(b.center.y - a.center.y, 2) + powf(b.center.z - a.center.z, 2) <= powf(b.radius + a.radius, 2); }

Vector3 Reflect(const Vector3& input, const Vector3& normal) { return input - normal * (2.0f * Dot(input, normal)); }

Vector3 CatmullRomInterpolation(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
	const float s = 0.5f;

	float t2 = t * t;
	float t3 = t2 * t;

	Vector3 e3 = -p0 + 3 * p1 - 3 * p2 + p3;
	Vector3 e2 = 2 * p0 - 5 * p1 + 4 * p2 - p3;
	Vector3 e1 = -p0 + p2;
	Vector3 e0 = 2 * p1;

	return s * (e3 * t3 + e2 * t2 + e1 * t + e0);
}

Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t) {
	assert(points.size() >= 4 && "制御点は4点以上必要です");

	// 区間数は制御点の数-1
	size_t division = points.size() - 1;
	// 1区間の長さ(全体を1.0とした割合)
	float areaWidth = 1.0f / division;

	// 区間内の始点を0.0f、終点を1.0fとしたときの現在位置
	float t_2 = std::fmod(t, areaWidth) * division;
	// 下限(0.0f)と上限(1.0f)の範囲に収める
	t_2 = std::clamp(t_2, 0.0f, 1.0f);

	// 区間番号
	size_t index = static_cast<size_t>(t / areaWidth);
	// 区間番号が上限を超えないように収める
	index = std::min<size_t>(index, division - 1);

	// 4点分のインデックス
	size_t index0 = index - 1;
	size_t index1 = index;
	size_t index2 = index + 1;
	size_t index3 = index + 2;

	// 最初の区間のp0はp1を重複使用する
	if (index == 0) {
		index0 = index1;
	}

	// 最後の区間のp3はp2を重複使用する
	if (index3 >= points.size()) {
		index3 = index2;
	}

	// 4点の座標
	const Vector3& p0 = points[index0];
	const Vector3& p1 = points[index1];
	const Vector3& p2 = points[index2];
	const Vector3& p3 = points[index3];

	// 4点を指定してCatmull-Rom補間
	return CatmullRomInterpolation(p0, p1, p2, p3, t_2);
}

Segment MakeLimitedSegment(const Vector3& start, const Vector3& end, float maxDistance) {
	Vector3 toEnd = end - start;

	Segment segment{};
	segment.origin = start;

	if (Length(toEnd) <= maxDistance) {
		segment.diff = toEnd;
	} else {
		Vector3 direction = Normalize(toEnd);
		segment.diff = direction * (maxDistance / (Length(toEnd))) * maxDistance;
	}

	return segment;
}

Segment MakeNattoSegment(const Vector3& start, const Vector3& end, float maxDistance, float minDistance) {
	Vector3 toEnd = end - start;

	Segment segment{};
	segment.origin = start;

	if (Length(toEnd) <= maxDistance) {
		segment.diff = toEnd;
	} else {
		Vector3 direction = Normalize(toEnd);
		segment.diff = direction * (maxDistance / Length(toEnd) * 4.0f);
	}

	return segment;
}

} // namespace ShapeUtil

} // namespace KujakuEngine
