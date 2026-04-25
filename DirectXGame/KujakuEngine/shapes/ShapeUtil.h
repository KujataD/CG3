#pragma once
#include "Rect.h"
#include "AABB.h"

namespace KujakuEngine {

// 図形仮置き　後々それぞれのヘッダーを分けます
struct Triangle {
	Vector3 vertices[3];
};

struct Spring {
	Vector3 anchor;           // アンカー
	float naturalLength;      // 自然調
	float stiffness;          // 剛性
	float dampingCoefficient; // 減衰係数
};

struct Ball {
	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;
	float mass;
	float radius;
	unsigned int color;
};

struct Pendulum {
	Vector3 anchor;
	float length;
	float angle;
	float angularVelocity;
	float angularAcceleration;
};

struct ConicalPendulum {
	Vector3 anchor;
	float length;
	float halfApexAngle;
	float angle;
	float angularVelocity;
};


struct Segment {
	Vector3 origin; // 始点
	Vector3 diff;   // 終点の差分ベクトル
};

struct Ray {
	Vector3 origin; // 始点
	Vector3 diff;   // 終点の差分ベクトル
};

struct Line {
	Vector3 origin; // 始点
	Vector3 diff;   // 終点の差分ベクトル
};

struct Sphere {
	Vector3 center;
	float radius;
};

struct Plane {
	Vector3 normal;
	float distance;
};
namespace ShapeUtil {


bool IsCollision(const Sphere& sphere, const Plane& plane);

bool IsCollision(const Segment& line, const Plane& plane);

bool IsCollision(const Segment& segment, const Triangle& triangle);

bool IsCollision(const AABB& aabb1, const AABB& aabb2);

bool IsCollision(const AABB& aabb, const Sphere& sphere);

bool IsCollision(const AABB& aabb, const Segment& segment);

bool IsCollision(const AABB& aabb, const Line& line);

bool IsCollision(const AABB& aabb, const Ray& ray);

bool IsCollision(const OBB& obb, const Sphere& sphere);

bool IsCollision(const OBB& obb, const Segment& segment);

bool IsCollision(const OBB& obb, const Line& line);

bool IsCollision(const OBB& obb, const Ray& ray);

bool IsCollision(const OBB& obb1, const OBB& obb2);

/// <summary>
/// 軸が重なっているかどうか
/// </summary>
bool IsOverlappingOnAxis(const OBB& A, const OBB& B, const Vector3& axis);

Vector3 Reflect(const Vector3& input, const Vector3& normal);


} // namespace ShapeUtil

} // namespace KujakuEngine