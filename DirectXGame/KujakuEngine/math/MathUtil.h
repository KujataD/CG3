#pragma once
#include <assert.h>
#include <cmath>
#include "../runtime/KujakuApi.h"
#include <math/Matrix3x3.h>
#include <math/Matrix4x4.h>
#include <math/Vector2.h>
#include <math/Vector3.h>
#include <math/Vector4.h>
#include <math/Random.h>
#include <numbers>


namespace KujakuEngine {
class Segment;

// --- vector2 ---

/// <summary>
/// Distanceを実行します。
/// </summary>
float Distance(const Vector2& v1, const Vector2& v2);

// --- vector3 ---

/// <summary>
/// operator+を実行します。
/// </summary>
inline Vector3 operator+(Vector3 v, float f) { return {v.x + f, v.y + f, v.z + f}; }
/// <summary>
/// operator-を実行します。
/// </summary>
inline Vector3 operator-(Vector3 v, float f) { return {v.x - f, v.y - f, v.z - f}; }
/// <summary>
/// operatorを実行します。
/// </summary>
inline Vector3 operator*(Vector3 v, float f) { return {v.x * f, v.y * f, v.z * f}; }
/// <summary>
/// operator/を実行します。
/// </summary>
inline Vector3 operator/(Vector3 v, float f) { return {v.x / f, v.y / f, v.z / f}; }
/// <summary>
/// operator+を実行します。
/// </summary>
inline Vector3 operator+(float f, Vector3 v) { return {v.x + f, v.y + f, v.z + f}; }
/// <summary>
/// operator-を実行します。
/// </summary>
inline Vector3 operator-(float f, Vector3 v) { return {v.x - f, v.y - f, v.z - f}; }
/// <summary>
/// operatorを実行します。
/// </summary>
inline Vector3 operator*(float f, Vector3 v) { return {v.x * f, v.y * f, v.z * f}; }

/// <summary>
/// Transformを実行します。
/// </summary>
Vector3 Transform(const Vector3& v, const Matrix4x4& m);

/// <summary>
/// Dotを実行します。
/// </summary>
float Dot(const Vector3& v1, const Vector3& v2);
/// <summary>
/// Crossを実行します。
/// </summary>
Vector3 Cross(const Vector3& a, const Vector3& b);
/// <summary>
/// Lengthを実行します。
/// </summary>
KUJAKU_API float Length(const Vector3& v);
/// <summary>
/// Normalizeを実行します。
/// </summary>
KUJAKU_API Vector3 Normalize(const Vector3& v);
/// <summary>
/// Lerpを実行します。
/// </summary>
float Lerp(float f1, float f2, float t);
/// <summary>
/// Lerpを実行します。
/// </summary>
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);
/// <summary>
/// Slerpを実行します。
/// </summary>
Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t);
/// <summary>
/// Bezierを実行します。
/// </summary>
Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);
/// <summary>
/// Reflectを実行します。
/// </summary>
Vector3 Reflect(const Vector3& input, const Vector3& normal);
/// <summary>
/// Projectを実行します。
/// </summary>
Vector3 Project(const Vector3& a, const Vector3& b);
/// <summary>
/// Projectを実行します。
/// </summary>
Vector3 Project(const Vector3& worldPosition, float viewportX, float viewportY, float viewportWidth, float viewportHeight, const Matrix4x4& matView, const Matrix4x4& matProjection);
/// <param name="point"></param>
/// <param name="segment"></param>
/// <returns></returns>
/// <summary>
/// ClosestPointを実行します。
/// </summary>
Vector3 ClosestPoint(const Vector3& point, const Segment& segment);

/// <param name="vector"></param>
/// <returns></returns>
/// <summary>
/// Perpendicularを実行します。
/// </summary>
Vector3 Perpendicular(const Vector3& vector);
/// <summary>
/// TransformNormalを実行します。
/// </summary>
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

/// <summary>
/// LookAtを実行します。
/// </summary>
Vector3 LookAt(const Vector3& velocity);

/// <summary>
/// WrapAngleを実行します。
/// </summary>
float WrapAngle(float angle);

/// <summary>
/// LerpShortAngleを実行します。
/// </summary>
float LerpShortAngle(float a, float b, float t);

/// <summary>
/// Limitを実行します。
/// </summary>
Vector3 Limit(Vector3 v, float max);

// --- matrix4x4 ---
/// <summary>
/// AffineMatrixOrientationsを生成します。
/// </summary>
Matrix4x4 MakeAffineMatrixOrientations(const Vector3 orientations[3], const Vector3& translate);

/// <summary>
/// Inverseを実行します。
/// </summary>
Matrix4x4 Inverse(const Matrix4x4& m);
/// <summary>
/// Transposeを実行します。
/// </summary>
Matrix4x4 Transpose(const Matrix4x4& m);
/// <summary>
/// Identityを生成します。
/// </summary>
Matrix4x4 MakeIdentity();

/// <summary>
/// TranslateMatrixを生成します。
/// </summary>
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
/// <summary>
/// ScaleMatrixを生成します。
/// </summary>
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
/// <summary>
/// RotateXMatrixを生成します。
/// </summary>
Matrix4x4 MakeRotateXMatrix(float radian);
/// <summary>
/// RotateYMatrixを生成します。
/// </summary>
KUJAKU_API Matrix4x4 MakeRotateYMatrix(float radian);
/// <summary>
/// RotateZMatrixを生成します。
/// </summary>
Matrix4x4 MakeRotateZMatrix(float radian);

/// <summary>
/// AffineMatrixを生成します。
/// </summary>
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
/// <summary>
/// RotateMatrixを生成します。
/// </summary>
Matrix4x4 MakeRotateMatrix( const Vector3& rotate);

// 透視投影行列
/// <summary>
/// PerspectiveFovMatrixを生成します。
/// </summary>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

// 正射影行列
/// <summary>
/// OrthographicMatrixを生成します。
/// </summary>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

// ビューポート変換行列
/// <summary>
/// ViewportMatrixを生成します。
/// </summary>
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

/// <summary>
/// BillboardMatrixを生成します。
/// </summary>
TransformationMatrix MakeBillboardMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate, const class Camera& camera);

inline Matrix4x4 kBackToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);

} // namespace KujakuEngine
