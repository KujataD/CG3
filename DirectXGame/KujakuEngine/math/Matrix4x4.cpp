#include "Matrix4x4.h"
#include "../3d/Camera.h"
#include "../3d/WorldTransform.h"

namespace KujakuEngine {
Matrix4x4 Matrix4x4::operator+(const Matrix4x4& other) const {
	return {
	    {
         {
	            this->m[0][0] + other.m[0][0],
	            this->m[0][1] + other.m[0][1],
	            this->m[0][2] + other.m[0][2],
	            this->m[0][3] + other.m[0][3],
	        }, {
	            this->m[1][0] + other.m[1][0],
	            this->m[1][1] + other.m[1][1],
	            this->m[1][2] + other.m[1][2],
	            this->m[1][3] + other.m[1][3],
	        }, {
	            this->m[2][0] + other.m[2][0],
	            this->m[2][1] + other.m[2][1],
	            this->m[2][2] + other.m[2][2],
	            this->m[2][3] + other.m[2][3],
	        }, {
	            this->m[3][0] + other.m[3][0],
	            this->m[3][1] + other.m[3][1],
	            this->m[3][2] + other.m[3][2],
	            this->m[3][3] + other.m[3][3],
	        }, }
    };
}

Matrix4x4 Matrix4x4::operator-(const Matrix4x4& other) const {
	return {
	    {
         {
	            this->m[0][0] - other.m[0][0],
	            this->m[0][1] - other.m[0][1],
	            this->m[0][2] - other.m[0][2],
	            this->m[0][3] - other.m[0][3],
	        }, {
	            this->m[1][0] - other.m[1][0],
	            this->m[1][1] - other.m[1][1],
	            this->m[1][2] - other.m[1][2],
	            this->m[1][3] - other.m[1][3],
	        }, {
	            this->m[2][0] - other.m[2][0],
	            this->m[2][1] - other.m[2][1],
	            this->m[2][2] - other.m[2][2],
	            this->m[2][3] - other.m[2][3],
	        }, {
	            this->m[3][0] - other.m[3][0],
	            this->m[3][1] - other.m[3][1],
	            this->m[3][2] - other.m[3][2],
	            this->m[3][3] - other.m[3][3],
	        }, }
    };
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
	return {
	    {
         {
	            this->m[0][0] * other.m[0][0] + this->m[0][1] * other.m[1][0] + this->m[0][2] * other.m[2][0] + this->m[0][3] * other.m[3][0],
	            this->m[0][0] * other.m[0][1] + this->m[0][1] * other.m[1][1] + this->m[0][2] * other.m[2][1] + this->m[0][3] * other.m[3][1],
	            this->m[0][0] * other.m[0][2] + this->m[0][1] * other.m[1][2] + this->m[0][2] * other.m[2][2] + this->m[0][3] * other.m[3][2],
	            this->m[0][0] * other.m[0][3] + this->m[0][1] * other.m[1][3] + this->m[0][2] * other.m[2][3] + this->m[0][3] * other.m[3][3],
	        }, {this->m[1][0] * other.m[0][0] + this->m[1][1] * other.m[1][0] + this->m[1][2] * other.m[2][0] + this->m[1][3] * other.m[3][0],
	         this->m[1][0] * other.m[0][1] + this->m[1][1] * other.m[1][1] + this->m[1][2] * other.m[2][1] + this->m[1][3] * other.m[3][1],
	         this->m[1][0] * other.m[0][2] + this->m[1][1] * other.m[1][2] + this->m[1][2] * other.m[2][2] + this->m[1][3] * other.m[3][2],
	         this->m[1][0] * other.m[0][3] + this->m[1][1] * other.m[1][3] + this->m[1][2] * other.m[2][3] + this->m[1][3] * other.m[3][3]},
         {
	            this->m[2][0] * other.m[0][0] + this->m[2][1] * other.m[1][0] + this->m[2][2] * other.m[2][0] + this->m[2][3] * other.m[3][0],
	            this->m[2][0] * other.m[0][1] + this->m[2][1] * other.m[1][1] + this->m[2][2] * other.m[2][1] + this->m[2][3] * other.m[3][1],
	            this->m[2][0] * other.m[0][2] + this->m[2][1] * other.m[1][2] + this->m[2][2] * other.m[2][2] + this->m[2][3] * other.m[3][2],
	            this->m[2][0] * other.m[0][3] + this->m[2][1] * other.m[1][3] + this->m[2][2] * other.m[2][3] + this->m[2][3] * other.m[3][3],
	        }, {
	            this->m[3][0] * other.m[0][0] + this->m[3][1] * other.m[1][0] + this->m[3][2] * other.m[2][0] + this->m[3][3] * other.m[3][0],
	            this->m[3][0] * other.m[0][1] + this->m[3][1] * other.m[1][1] + this->m[3][2] * other.m[2][1] + this->m[3][3] * other.m[3][1],
	            this->m[3][0] * other.m[0][2] + this->m[3][1] * other.m[1][2] + this->m[3][2] * other.m[2][2] + this->m[3][3] * other.m[3][2],
	            this->m[3][0] * other.m[0][3] + this->m[3][1] * other.m[1][3] + this->m[3][2] * other.m[2][3] + this->m[3][3] * other.m[3][3],
	        }, }
    };
}

Matrix4x4 Matrix4x4::operator/(float scalar) const {
	return {
	    {{this->m[0][0] / scalar, this->m[0][1] / scalar, this->m[0][2] / scalar, this->m[0][3] / scalar},
	     {this->m[1][0] / scalar, this->m[1][1] / scalar, this->m[1][2] / scalar, this->m[1][3] / scalar},
	     {this->m[2][0] / scalar, this->m[2][1] / scalar, this->m[2][2] / scalar, this->m[2][3] / scalar},
	     {this->m[3][0] / scalar, this->m[3][1] / scalar, this->m[3][2] / scalar, this->m[3][3] / scalar}}
    };
}

Matrix4x4 Matrix4x4::MakeAffineMatrixOrientations(const Vector3 orientations[3], const Vector3& translate) {
	return {
	    {{orientations[0].x, orientations[0].y, orientations[0].z, 0.0f},
	     {orientations[1].x, orientations[1].y, orientations[1].z, 0.0f},
	     {orientations[2].x, orientations[2].y, orientations[2].z, 0.0f},
	     {translate.x, translate.y, translate.z, 1.0f}}
    };
}
Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& m) {
	Matrix4x4 result;

	result.m[0][0] = m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[1][2] * m.m[2][1] * m.m[3][3] -
	                 m.m[1][3] * m.m[2][2] * m.m[3][1];

	result.m[0][1] = -m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][3] * m.m[3][2] +
	                 m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][3] * m.m[2][2] * m.m[3][1];

	result.m[0][2] = m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][3] * m.m[3][2] - m.m[0][2] * m.m[1][1] * m.m[3][3] -
	                 m.m[0][3] * m.m[1][2] * m.m[3][1];

	result.m[0][3] = -m.m[0][1] * m.m[1][2] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][3] * m.m[2][2] +
	                 m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][3] * m.m[1][2] * m.m[2][1];

	result.m[1][0] = -m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][3] * m.m[3][2] +
	                 m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][3] * m.m[2][2] * m.m[3][0];

	result.m[1][1] = m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][3] * m.m[3][2] - m.m[0][2] * m.m[2][0] * m.m[3][3] -
	                 m.m[0][3] * m.m[2][2] * m.m[3][0];

	result.m[1][2] = -m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][3] * m.m[3][2] +
	                 m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][3] * m.m[1][2] * m.m[3][0];

	result.m[1][3] = m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] - m.m[0][2] * m.m[1][0] * m.m[2][3] -
	                 m.m[0][3] * m.m[1][2] * m.m[2][0];

	result.m[2][0] = m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1] - m.m[1][0] * m.m[2][3] * m.m[3][1] - m.m[1][1] * m.m[2][0] * m.m[3][3] -
	                 m.m[1][3] * m.m[2][1] * m.m[3][0];

	result.m[2][1] = -m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1] + m.m[0][0] * m.m[2][3] * m.m[3][1] +
	                 m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][3] * m.m[2][1] * m.m[3][0];

	result.m[2][2] = m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1] - m.m[0][0] * m.m[1][3] * m.m[3][1] - m.m[0][1] * m.m[1][0] * m.m[3][3] -
	                 m.m[0][3] * m.m[1][1] * m.m[3][0];

	result.m[2][3] = -m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] +
	                 m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][3] * m.m[1][1] * m.m[2][0];

	result.m[3][0] = -m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1] + m.m[1][0] * m.m[2][2] * m.m[3][1] +
	                 m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][2] * m.m[2][1] * m.m[3][0];

	result.m[3][1] = m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1] - m.m[0][0] * m.m[2][2] * m.m[3][1] - m.m[0][1] * m.m[2][0] * m.m[3][2] -
	                 m.m[0][2] * m.m[2][1] * m.m[3][0];

	result.m[3][2] = -m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1] + m.m[0][0] * m.m[1][2] * m.m[3][1] +
	                 m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][2] * m.m[1][1] * m.m[3][0];

	result.m[3][3] = m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][0] * m.m[1][2] * m.m[2][1] - m.m[0][1] * m.m[1][0] * m.m[2][2] -
	                 m.m[0][2] * m.m[1][1] * m.m[2][0];

	float det;
	det =
	    (m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2] - m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1] -
	     m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2] - m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1] -
	     m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2] + m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2] +
	     m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2] - m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1] -
	     m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0] -
	     m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0] + m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0]);

	result = result / det;

	return result;
}

Matrix4x4 Matrix4x4::Transpose(const Matrix4x4& m) {
	Matrix4x4 result;
	result = {
	    {
         {
	            m.m[0][0],
	            m.m[1][0],
	            m.m[2][0],
	            m.m[3][0],
	        }, {
	            m.m[0][1],
	            m.m[1][1],
	            m.m[2][1],
	            m.m[3][1],
	        }, {
	            m.m[0][2],
	            m.m[1][2],
	            m.m[2][2],
	            m.m[3][2],
	        }, {
	            m.m[0][3],
	            m.m[1][3],
	            m.m[2][3],
	            m.m[3][3],
	        }, }
    };
	return result;
}

Matrix4x4 Matrix4x4::MakeIdentity() {
	return {
	    {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeTranslateMatrix(const Vector3& translate) {
	return {
	    {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {translate.x, translate.y, translate.z, 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeScaleMatrix(const Vector3& scale) {
	return {
	    {{scale.x, 0.0f, 0.0f, 0.0f}, {0.0f, scale.y, 0.0f, 0.0f}, {0.0f, 0.0f, scale.z, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeRotateXMatrix(float radian) {
	return {
	    {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, std::cos(radian), std::sin(radian), 0.0f}, {0.0f, -std::sin(radian), std::cos(radian), 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeRotateYMatrix(float radian) {
	return {
	    {{std::cos(radian), 0.0f, -std::sin(radian), 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {std::sin(radian), 0.0f, std::cos(radian), 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeRotateZMatrix(float radian) {
	return {
	    {{std::cos(radian), std::sin(radian), 0.0f, 0.0f}, {-std::sin(radian), std::cos(radian), 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 s = MakeScaleMatrix(scale);
	Matrix4x4 r = MakeRotateXMatrix(rotate.x) * MakeRotateYMatrix(rotate.y) * MakeRotateZMatrix(rotate.z);
	Matrix4x4 t = MakeTranslateMatrix(translate);
	Matrix4x4 w = s * r * t;
	return w;
}

Matrix4x4 Matrix4x4::MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	return {
	    {{(1.0f / std::tan(fovY / 2.0f)) / aspectRatio, 0.0f, 0.0f, 0.0f},
	     {0.0f, (1.0f / std::tan(fovY / 2.0f)), 0.0f, 0.0f},
	     {0.0f, 0.0f, farClip / (farClip - nearClip), 1.0f},
	     {0.0f, 0.0f, -nearClip * farClip / (farClip - nearClip), 0.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	return {
	    {{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
	     {0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
	     {0.0f, 0.0f, 1.0f / (farClip - nearClip), 0.0f},
	     {(left + right) / (left - right), (top + bottom) / (bottom - top), nearClip / (nearClip - farClip), 1.0f}}
    };
}

Matrix4x4 Matrix4x4::MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	if (minDepth > maxDepth) {
		float tmp = minDepth;
		minDepth = maxDepth;
		maxDepth = tmp;
	}

	return {
	    {{width / 2.0f, 0.0f, 0.0f, 0.0f}, {0.0f, -(height / 2.0f), 0.0f, 0.0f}, {0.0f, 0.0f, maxDepth - minDepth, 0.0f}, {left + (width / 2.0f), top + (height / 2.0f), minDepth, 1.0f}}
    };
}
TransformationMatrix Matrix4x4::MakeBillboardMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate, const Camera& camera) { // ワールド行列の生成
	Matrix4x4 billboardMatrix = kBackToFrontMatrix * Matrix4x4::Inverse(camera.matView);
	billboardMatrix.m[3][0] = 0.0f; // 平行移動成分は要らない
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	// 回転対応
	Matrix4x4 rotateZMatrix = Matrix4x4::MakeRotateZMatrix(rotate.z);

	Matrix4x4 rotateMatrix = rotateZMatrix * billboardMatrix;
	Matrix4x4 scaleMatrix = Matrix4x4::MakeScaleMatrix(scale);
	Matrix4x4 translateMatrix = Matrix4x4::MakeTranslateMatrix(translate);

	Matrix4x4 worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;

	TransformationMatrix result;
	result.World = worldMatrix;
	result.WVP = worldMatrix * camera.matView * camera.matProjection;

	return result;
}
} // namespace KujakuEngine