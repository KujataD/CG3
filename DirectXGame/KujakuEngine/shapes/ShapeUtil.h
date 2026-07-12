#pragma once
#include "AABB.h"
#include "Rect.h"
#include <vector>
#include <math/MathUtil.h>

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

class Segment {
public:
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

bool IsCollision(const AABB& aabb, const Vector3& point);

bool IsCollision(const AABB& aabb, const Segment& segment);

bool IsCollision(const AABB& aabb, const Line& line);

bool IsCollision(const AABB& aabb, const Ray& ray);

bool IsCollision(const OBB& obb, const Sphere& sphere);

bool IsCollision(const OBB& obb, const Segment& segment);

bool IsCollision(const OBB& obb, const Line& line);

bool IsCollision(const OBB& obb, const Ray& ray);

bool IsCollision(const OBB& obb1, const OBB& obb2);

bool IsCollision(const Sphere& a, const Sphere& b);

/// <summary>
/// 軸が重なっているかどうか
/// </summary>
bool IsOverlappingOnAxis(const OBB& A, const OBB& B, const Vector3& axis);

Vector3 Reflect(const Vector3& input, const Vector3& normal);

///< summary>
/// CatmullRom補間
///</summary>
///< param name="po">点0の座標</param>
///< param name="p1">点1の座標</param>
///< param name="p2">点2の座標</param>
///< param name="p3">点3の座標</param>
///< param name="t">点1を0.0f、点2を1.0fとした割合指定</param>
///< returns>点1と点2の間で指定された座標</returns>
Vector3 CatmullRomInterpolation(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

///< summary>
/// CatmullRomスプライン曲線上の座標を得る
///  </summary>
///< param name="points">制御点の集合</param>
/// <param name="t">スプラインの全区間の中での割合指定[0,1]</param>
///< returns>座標</returns>
Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t);

Segment MakeLimitedSegment(const Vector3& start, const Vector3& end, float maxDistance);
Segment MakeNattoSegment(const Vector3& start, const Vector3& end, float maxDistance, float minDistance);

} // namespace ShapeUtil

} // namespace KujakuEngine