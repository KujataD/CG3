#pragma once
#include "Vector2.h"


namespace KujakuEngine{

/// <summary>
/// Matrix3x3クラスを表します。
/// </summary>
class Matrix3x3 {
public:
	float m[3][3];

	/// <summary>
	/// Matrix3x3を実行します。
	/// </summary>
	Matrix3x3();
	/// <summary>
	/// Matrix3x3を実行します。
	/// </summary>
	Matrix3x3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22);

	// 演算子オーバーロード
	/// <summary>
	/// operator+を実行します。
	/// </summary>
	Matrix3x3 operator+(float f) const;
	/// <summary>
	/// operator-を実行します。
	/// </summary>
	Matrix3x3 operator-(float f) const;
	/// <summary>
	/// operatorを実行します。
	/// </summary>
	Matrix3x3 operator*(float f) const;
	/// <summary>
	/// operator/を実行します。
	/// </summary>
	Matrix3x3 operator/(float f) const;
	/// <summary>
	/// operatorを実行します。
	/// </summary>
	Matrix3x3 operator*(const Matrix3x3& other) const;

	// 行列生成
	/// <summary>
	/// ScaleMatrixを生成します。
	/// </summary>
	static Matrix3x3 MakeScaleMatrix(const Vector2& scale);
	/// <summary>
	/// RotateMatrixを生成します。
	/// </summary>
	static Matrix3x3 MakeRotateMatrix(float theta);
	/// <summary>
	/// TranslateMatrixを生成します。
	/// </summary>
	static Matrix3x3 MakeTranslateMatrix(const Vector2& translate);
	/// <summary>
	/// AffineMatrixを生成します。
	/// </summary>
	static Matrix3x3 MakeAffineMatrix(const Vector2& scale, float rotate, const Vector2& translate);
	/// <summary>
	/// OrthographicMatrixを生成します。
	/// </summary>
	static Matrix3x3 MakeOrthographicMatrix(float left, float top, float right, float bottom);
	/// <summary>
	/// ViewportMatrixを生成します。
	/// </summary>
	static Matrix3x3 MakeViewportMatrix(float left, float top, float width, float height);

	// その他
	/// <summary>
	/// Inverseを実行します。
	/// </summary>
	static Matrix3x3 Inverse(const Matrix3x3& m);
	/// <summary>
	/// Transposeを実行します。
	/// </summary>
	static Matrix3x3 Transpose(const Matrix3x3& m);
};
}