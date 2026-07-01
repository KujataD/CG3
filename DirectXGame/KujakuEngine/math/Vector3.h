#pragma once
#include <assert.h>
#include <cmath>
#include <numbers>

namespace KujakuEngine {

class Matrix4x4;
class Segment;

/// <summary>
/// Vector3クラスを表します。
/// </summary>
class Vector3 {
public:
	float x;
	float y;
	float z;

	/// <summary>
	/// operator+を実行します。
	/// </summary>
	Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
	/// <summary>
	/// operator-を実行します。
	/// </summary>
	Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
	/// <summary>
	/// operator-を実行します。
	/// </summary>
	Vector3 operator-() const { return {-x, -y, -z}; }

	void operator+=(const Vector3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
	}
	void operator-=(const Vector3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}
	void operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}
	void operator/=(float scalar) {
		x /= scalar;
		y /= scalar;
		z /= scalar;
	}
};

} // namespace KujakuEngine