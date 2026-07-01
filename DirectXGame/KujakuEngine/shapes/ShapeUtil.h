#pragma once
#include "AABB.h"
#include "Rect.h"
#include <vector>
#include "../vfx/InstancingModel.h"
#include "../3d/Camera.h"
#include <math/MathUtil.h>

namespace KujakuEngine {

// 図形仮置き　後々それぞれのヘッダーを分けます
/// <summary>
/// Triangle構造体を表します。
/// </summary>
struct Triangle {
	Vector3 vertices[3];
};

/// <summary>
/// Spring構造体を表します。
/// </summary>
struct Spring {
	Vector3 anchor;           // アンカー
	float naturalLength;      // 自然調
	float stiffness;          // 剛性
	float dampingCoefficient; // 減衰係数
};

/// <summary>
/// Ball構造体を表します。
/// </summary>
struct Ball {
	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;
	float mass;
	float radius;
	unsigned int color;
};

/// <summary>
/// Pendulum構造体を表します。
/// </summary>
struct Pendulum {
	Vector3 anchor;
	float length;
	float angle;
	float angularVelocity;
	float angularAcceleration;
};

/// <summary>
/// ConicalPendulum構造体を表します。
/// </summary>
struct ConicalPendulum {
	Vector3 anchor;
	float length;
	float halfApexAngle;
	float angle;
	float angularVelocity;
};

/// <summary>
/// Segmentクラスを表します。
/// </summary>
class Segment {
public:
	Vector3 origin; // 始点
	Vector3 diff;   // 終点の差分ベクトル
};

/// <summary>
/// Ray構造体を表します。
/// </summary>
struct Ray {
	Vector3 origin; // 始点
	Vector3 diff;   // 終点の差分ベクトル
};

/// <summary>
/// Line構造体を表します。
/// </summary>
struct Line {
	Vector3 origin; // 始点
	Vector3 diff;   // 終点の差分ベクトル
};

/// <summary>
/// Sphere構造体を表します。
/// </summary>
struct Sphere {
	Vector3 center;
	float radius;
};

/// <summary>
/// Plane構造体を表します。
/// </summary>
struct Plane {
	Vector3 normal;
	float distance;
};
namespace ShapeUtil {

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const Sphere& sphere, const Plane& plane);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const Segment& line, const Plane& plane);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const Segment& segment, const Triangle& triangle);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const AABB& aabb1, const AABB& aabb2);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const AABB& aabb, const Sphere& sphere);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const AABB& aabb, const Vector3& point);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const AABB& aabb, const Segment& segment);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const AABB& aabb, const Line& line);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const AABB& aabb, const Ray& ray);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const OBB& obb, const Sphere& sphere);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const OBB& obb, const Segment& segment);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const OBB& obb, const Line& line);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const OBB& obb, const Ray& ray);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const OBB& obb1, const OBB& obb2);

/// <summary>
/// Collisionかどうかを返します。
/// </summary>
bool IsCollision(const Sphere& a, const Sphere& b);

/// <summary>
/// 軸が重なっているかどうか
/// </summary>
bool IsOverlappingOnAxis(const OBB& A, const OBB& B, const Vector3& axis);

/// <summary>
/// Reflectを実行します。
/// </summary>
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
/// <summary>
/// CatmullRomInterpolationを実行します。
/// </summary>
Vector3 CatmullRomInterpolation(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

///< summary>
/// CatmullRomスプライン曲線上の座標を得る
///  </summary>
///< param name="points">制御点の集合</param>
/// <param name="t">スプラインの全区間の中での割合指定[0,1]</param>
///< returns>座標</returns>
/// <summary>
/// CatmullRomPositionを実行します。
/// </summary>
Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t);

/// <summary>
/// SplineParticlesを描画します。
/// </summary>
void DrawSplineParticles(InstancingModel* model, const std::vector<Vector3>& controlPoints, const Camera& camera);

/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="maxDistance"></param>
/// <returns></returns>
/// <summary>
/// LimitedSegmentを生成します。
/// </summary>
Segment MakeLimitedSegment(const Vector3& start, const Vector3& end, float maxDistance);
/// <summary>
/// NattoSegmentを生成します。
/// </summary>
Segment MakeNattoSegment(const Vector3& start, const Vector3& end, float maxDistance, float minDistance);

} // namespace ShapeUtil

} // namespace KujakuEngine