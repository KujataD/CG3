#pragma once
#include <assert.h>
#include <cmath>

namespace KujakuEngine {

class Matrix4x4;
class Segment;

class Vector3 {
public:
	float x;
	float y;
	float z;

	Vector3 operator+(const Vector3& v) const { return {x + v.x, y + v.y, z + v.z}; }
	Vector3 operator-(const Vector3& v) const { return {x - v.x, y - v.y, z - v.z}; }
	Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
	Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }
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

	static float Dot(const Vector3& v1, const Vector3& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
	static Vector3 Cross(const Vector3& a, const Vector3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
	static float Length(const Vector3& v) { return std::sqrt(Dot(v, v)); }
	static Vector3 Normalize(const Vector3& v) { return v / Length(v); }
	static Vector3 Lerp(const Vector3& v1, const Vector3 v2, float t);
	static Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);
	static Vector3 Reflect(const Vector3& input, const Vector3& normal);

	static Vector3 Transform(const Vector3& v, const Matrix4x4& m);
	static Vector3 Project(const Vector3& a, const Vector3& b);
	static Vector3 ClosestPoint(const Vector3& point, const Segment& segment);
	static Vector3 Perpendicular(const Vector3& vector);
};

} // namespace KujakuEngine