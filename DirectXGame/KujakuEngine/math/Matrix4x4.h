#pragma once
#include "Vector3.h"
#include <algorithm>
#include <assert.h>
#include <numbers>
#include <cmath>

namespace KujakuEngine {
class TransformationMatrix;
class Matrix4x4 {
public:
	float m[4][4];

	Matrix4x4 operator+(const Matrix4x4& m) const;
	Matrix4x4 operator-(const Matrix4x4& m) const;
	Matrix4x4 operator*(const Matrix4x4& m) const;
	Matrix4x4 operator/(float scalar) const;

	static Matrix4x4 MakeAffineMatrixOrientations(const Vector3 orientations[3], const Vector3& translate);

	static Matrix4x4 Inverse(const Matrix4x4& m);
	static Matrix4x4 Transpose(const Matrix4x4& m);
	static Matrix4x4 MakeIdentity();

	static Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);
	static Matrix4x4 MakeRotateXMatrix(float radian);
	static Matrix4x4 MakeRotateYMatrix(float radian);
	static Matrix4x4 MakeRotateZMatrix(float radian);

	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	// 透視投影行列
	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	// 正射影行列
	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	// ビューポート変換行列
	static Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

	static TransformationMatrix MakeBillboardMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate, const class Camera& camera);
};

static inline Matrix4x4 kBackToFrontMatrix = Matrix4x4::MakeRotateYMatrix(std::numbers::pi_v<float>);
} // namespace KujakuEngine