#include "TestFramework.h"

#include "math/MathUtil.h"

using namespace KujakuEngine;

KU_TEST(Dot_内積) {
	KU_CHECK_NEAR(Dot(Vector3{1, 0, 0}, Vector3{0, 1, 0}), 0.0f, 1e-5f);   // 直交 → 0
	KU_CHECK_NEAR(Dot(Vector3{2, 3, 0}, Vector3{2, 3, 0}), 13.0f, 1e-5f);  // 2*2 + 3*3
	KU_CHECK_NEAR(Dot(Vector3{1, 2, 3}, Vector3{4, 5, 6}), 32.0f, 1e-5f);  // 4 + 10 + 18
}

KU_TEST(Cross_外積) {
	Vector3 c = Cross(Vector3{1, 0, 0}, Vector3{0, 1, 0});  // x × y = z
	KU_CHECK_NEAR(c.x, 0.0f, 1e-5f);
	KU_CHECK_NEAR(c.y, 0.0f, 1e-5f);
	KU_CHECK_NEAR(c.z, 1.0f, 1e-5f);
}
