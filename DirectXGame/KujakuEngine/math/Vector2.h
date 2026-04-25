#pragma once
#include <cmath>
#include <math.h>

namespace KujakuEngine{

class Matrix3x3;

class Vector2 {
public:
	float x;
	float y;

	Vector2();
	Vector2(float x, float y);

	// 便利関数
	// ----------------------------------------------------

	static float Length(const Vector2& v);

	static Vector2 Normalize(const Vector2& v);

	static float Dot(const Vector2& a, const Vector2& b);

	/// <summary>
	/// 点pが線分abに対して左右どちらにあるか
	/// </summary>
	/// <param name="a">線分の始点</param>
	/// <param name="b">線分の終点</param>
	/// <param name="p">ターゲット</param>
	/// <returns>左にある場合 (Orientation(a,b,p) > 0.0f) が true </returns>
	static float Orientation(const Vector2& a, const Vector2& b, const Vector2& p);

	// 演算子オーバーロード
	// ----------------------------------------------------
	Vector2 operator+(float f) const;
	Vector2 operator+(const Vector2& other) const;
	Vector2& operator+=(float f);
	Vector2& operator+=(const Vector2& other);

	Vector2 operator-(float f) const;
	Vector2 operator-(const Vector2& other) const;
	Vector2& operator-=(float f);
	Vector2& operator-=(const Vector2& other);

	Vector2 operator*(float f);
	Vector2& operator*=(float f);
	Vector2 operator*(const Vector2& other) const;
	Vector2& operator*=(const Vector2& other);

	Vector2 operator/(float f);
	Vector2& operator/=(float f);
	Vector2 operator/(const Vector2& other) const;
	Vector2& operator/=(const Vector2& other);

	// 行列変換
	Vector2 Transform(const Matrix3x3& m) const;
};

}