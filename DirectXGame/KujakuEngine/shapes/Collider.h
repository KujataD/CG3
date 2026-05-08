#pragma once

#include "Rect.h"
#include "AABB.h"
#include "ShapeUtil.h"
#include "../math/Vector3.h"

namespace KujakuEngine{

class Collider {
public:
	// 半径を取得
	float GetRadius() const { return radius_; }

	// 半径を設定
	void SetRadius(float radius) { radius_ = radius; }

	// 衝突時に呼ばれる関数
	virtual void OnCollision() = 0;
	virtual const Vector3& GetWorldPosition() = 0;

private:
	// 衝突判定
	float radius_ = 1.0f;
};

}