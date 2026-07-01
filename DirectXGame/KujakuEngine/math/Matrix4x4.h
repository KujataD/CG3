#pragma once
#include "Vector3.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <numbers>

namespace KujakuEngine {
struct TransformationMatrix;
/// <summary>
/// Matrix4x4クラスを表します。
/// </summary>
class Matrix4x4 {
public:
	float m[4][4];

	/// <summary>
	/// operator+を実行します。
	/// </summary>
	Matrix4x4 operator+(const Matrix4x4& m) const;
	/// <summary>
	/// operator-を実行します。
	/// </summary>
	Matrix4x4 operator-(const Matrix4x4& m) const;
	/// <summary>
	/// operatorを実行します。
	/// </summary>
	Matrix4x4 operator*(const Matrix4x4& m) const;
	/// <summary>
	/// operator/を実行します。
	/// </summary>
	Matrix4x4 operator/(float scalar) const;
};
} // namespace KujakuEngine