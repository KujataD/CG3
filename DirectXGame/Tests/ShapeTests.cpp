#include "TestFramework.h"

#include "shapes/ShapeUtil.h"

using namespace KujakuEngine;

KU_TEST(衝突_Sphere同士) {
	Sphere a{{0, 0, 0}, 1.0f};
	Sphere b{{1.5f, 0, 0}, 1.0f};  // 中心間1.5 < 半径和2.0 → 衝突
	KU_CHECK(ShapeUtil::IsCollision(a, b) == true);

	Sphere c{{3.0f, 0, 0}, 1.0f};  // 中心間3.0 > 2.0 → 非衝突
	KU_CHECK(ShapeUtil::IsCollision(a, c) == false);
}

KU_TEST(衝突_AABB同士) {
	AABB a{{0, 0, 0}, {2, 2, 2}};
	AABB b{{1, 1, 1}, {3, 3, 3}};  // 重なる
	KU_CHECK(ShapeUtil::IsCollision(a, b) == true);

	AABB c{{5, 5, 5}, {6, 6, 6}};  // 離れている
	KU_CHECK(ShapeUtil::IsCollision(a, c) == false);
}
