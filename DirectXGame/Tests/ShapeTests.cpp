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

// --- Contact(接触情報: normal は第1→第2への分離方向、depth はめり込み量) ---

KU_TEST(接触_Sphere同士) {
	Sphere a{{0, 0, 0}, 1.0f};
	Sphere b{{1.5f, 0, 0}, 1.0f};  // 中心間1.5、半径和2.0 → depth 0.5
	Contact c{};
	KU_CHECK(ShapeUtil::ComputeContact(a, b, c) == true);
	KU_CHECK_NEAR(c.depth, 0.5f, 1e-4f);
	KU_CHECK_NEAR(c.normal.x, 1.0f, 1e-4f);  // a→b は +x
	KU_CHECK_NEAR(c.normal.y, 0.0f, 1e-4f);
	KU_CHECK_NEAR(c.normal.z, 0.0f, 1e-4f);
	KU_CHECK_NEAR(c.point.x, 0.75f, 1e-4f);  // a.center + n*(r - depth/2)

	Sphere far{{3.0f, 0, 0}, 1.0f};  // 非接触
	Contact c2{};
	KU_CHECK(ShapeUtil::ComputeContact(a, far, c2) == false);
}

KU_TEST(接触_SphereとOBB) {
	// 原点中心・軸並行・半径extent1 のOBB
	OBB obb{};
	obb.center = {0, 0, 0};
	obb.orientations[0] = {1, 0, 0};
	obb.orientations[1] = {0, 1, 0};
	obb.orientations[2] = {0, 0, 1};
	obb.size = {1, 1, 1};

	Sphere sphere{{1.8f, 0, 0}, 1.0f};  // 最近点(1,0,0)まで0.8 < 半径1 → depth 0.2
	Contact c{};
	KU_CHECK(ShapeUtil::ComputeContact(sphere, obb, c) == true);
	KU_CHECK_NEAR(c.depth, 0.2f, 1e-4f);
	KU_CHECK_NEAR(c.normal.x, -1.0f, 1e-4f);  // sphere→obb は -x
	KU_CHECK_NEAR(c.point.x, 1.0f, 1e-4f);

	Sphere out{{3.0f, 0, 0}, 1.0f};  // 非接触
	Contact c2{};
	KU_CHECK(ShapeUtil::ComputeContact(out, obb, c2) == false);
}

KU_TEST(接触_OBB同士) {
	OBB a{};
	a.center = {0, 0, 0};
	a.orientations[0] = {1, 0, 0};
	a.orientations[1] = {0, 1, 0};
	a.orientations[2] = {0, 0, 1};
	a.size = {1, 1, 1};

	OBB b = a;
	b.center = {1.5f, 0, 0};  // x方向に重なり0.5(最小めり込み軸=x)
	Contact c{};
	KU_CHECK(ShapeUtil::ComputeContact(a, b, c) == true);
	KU_CHECK_NEAR(c.depth, 0.5f, 1e-4f);
	KU_CHECK_NEAR(c.normal.x, 1.0f, 1e-4f);  // a→b は +x
	KU_CHECK_NEAR(c.normal.y, 0.0f, 1e-4f);
	KU_CHECK_NEAR(c.normal.z, 0.0f, 1e-4f);

	OBB far = a;
	far.center = {3.0f, 0, 0};  // 非接触
	Contact c2{};
	KU_CHECK(ShapeUtil::ComputeContact(a, far, c2) == false);
}
