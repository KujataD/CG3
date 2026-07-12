#include "TestFramework.h"

#include "runtime/TagRegistry.h"

using namespace KujakuEngine;

KU_TEST(Tag_登録と重複排除) {
	TagRegistry& registry = TagRegistry::GetInstance();

	KU_CHECK(registry.HasTag("Untagged") == true);        // 既定タグ
	KU_CHECK(registry.HasTag("__ku_test_tag__") == false);

	KU_CHECK(registry.AddTag("__ku_test_tag__") == true); // 新規追加
	KU_CHECK(registry.HasTag("__ku_test_tag__") == true);

	KU_CHECK(registry.AddTag("__ku_test_tag__") == false); // 重複は追加しない
	KU_CHECK(registry.AddTag("") == false);                // 空文字は無視

	// EnsureTag は重複を作らない
	size_t before = registry.GetTags().size();
	registry.EnsureTag("__ku_test_tag__");
	KU_CHECK(registry.GetTags().size() == before);
}
