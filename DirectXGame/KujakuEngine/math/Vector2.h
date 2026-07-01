#pragma once
#include "../runtime/KujakuApi.h"
#include <cmath>
#include <math.h>

namespace KujakuEngine{

class Matrix3x3;

/// <summary>
/// KUJAKU_APIクラスを表します。
/// </summary>
class KUJAKU_API Vector2 {
public:
	float x;
	float y;

	/// <summary>
	/// Vector2を実行します。
	/// </summary>
	Vector2();
	/// <summary>
	/// Vector2を実行します。
	/// </summary>
	Vector2(float x, float y);

	// 便利関数
	// ----------------------------------------------------

	/// <summary>
	/// Lengthを実行します。
	/// </summary>
	static float Length(const Vector2& v);

	/// <summary>
	/// Normalizeを実行します。
	/// </summary>
	static Vector2 Normalize(const Vector2& v);

	/// <summary>
	/// Dotを実行します。
	/// </summary>
	static float Dot(const Vector2& a, const Vector2& b);

	/// <param name="a">線分の始点</param>
	/// <param name="b">線分の終点</param>
	/// <param name="p">ターゲット</param>
	/// <returns>左にある場合 (Orientation(a,b,p) > 0.0f) が true </returns>
	/// <summary>
	/// Orientationを実行します。
	/// </summary>
	static float Orientation(const Vector2& a, const Vector2& b, const Vector2& p);

	// 演算子オーバーロード
	// ----------------------------------------------------
	/// <summary>
	/// operator+を実行します。
	/// </summary>
	Vector2 operator+(float f) const;
	/// <summary>
	/// operator+を実行します。
	/// </summary>
	Vector2 operator+(const Vector2& other) const;
	Vector2& operator+=(float f);
	Vector2& operator+=(const Vector2& other);

	/// <summary>
	/// operator-を実行します。
	/// </summary>
	Vector2 operator-(float f) const;
	/// <summary>
	/// operator-を実行します。
	/// </summary>
	Vector2 operator-(const Vector2& other) const;
	Vector2& operator-=(float f);
	Vector2& operator-=(const Vector2& other);

	/// <summary>
	/// operatorを実行します。
	/// </summary>
	Vector2 operator*(float f);
	Vector2& operator*=(float f);
	/// <summary>
	/// operatorを実行します。
	/// </summary>
	Vector2 operator*(const Vector2& other) const;
	Vector2& operator*=(const Vector2& other);

	/// <summary>
	/// operator/を実行します。
	/// </summary>
	Vector2 operator/(float f);
	Vector2& operator/=(float f);
	/// <summary>
	/// operator/を実行します。
	/// </summary>
	Vector2 operator/(const Vector2& other) const;
	Vector2& operator/=(const Vector2& other);

	bool operator!=(const Vector2& other) const;
	bool operator==(const Vector2& other) const;

	// 行列変換
	/// <summary>
	/// Transformを実行します。
	/// </summary>
	Vector2 Transform(const Matrix3x3& m) const;
};

}
