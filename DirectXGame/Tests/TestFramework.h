#pragma once

// 依存ゼロの軽量テストハーネス。
//   KU_TEST(名前) { ... } でテストを定義し、KU_CHECK / KU_CHECK_NEAR で検証する。
//   TestMain.cpp の kutest::RunAll() が全テストを実行し、失敗数を返す(CI用: 0=成功)。
//   将来 doctest / Catch2 へ移行したくなったらマクロを差し替えるだけでよい。

#include <cmath>
#include <cstdio>
#include <vector>

namespace kutest {

using TestFn = void (*)();

struct TestCase {
	const char* name;
	TestFn fn;
};

inline std::vector<TestCase>& Registry() {
	static std::vector<TestCase> registry;
	return registry;
}

inline int& CheckCount() {
	static int count = 0;
	return count;
}

inline int& FailCount() {
	static int count = 0;
	return count;
}

// KU_TEST が生成する登録子。コンストラクタでテストをレジストリへ積む。
struct Registrar {
	Registrar(const char* name, TestFn fn) { Registry().push_back({name, fn}); }
};

inline void ReportFail(const char* file, int line, const char* expr) {
	std::printf("    [FAIL] %s:%d  %s\n", file, line, expr);
	++FailCount();
}

// 全テストを実行し、失敗チェック数を返す(0なら全成功)。
inline int RunAll() {
	std::printf("=== KujakuEngine Tests ===\n%zu 件のテストを実行します。\n\n", Registry().size());
	for (const TestCase& test : Registry()) {
		std::printf("- %s\n", test.name);
		int failsBefore = FailCount();
		test.fn();
		if (FailCount() == failsBefore) {
			std::printf("    OK\n");
		}
	}
	std::printf("\n--- 結果: %d チェック中 %d 失敗 ---\n", CheckCount(), FailCount());
	return FailCount() == 0 ? 0 : 1;
}

} // namespace kutest

// テスト定義:  KU_TEST(識別子) { ... 検証 ... }
#define KU_TEST(name)                                                  \
	static void name();                                                \
	static const ::kutest::Registrar ku_registrar_##name{#name, name}; \
	static void name()

// 条件が真であることを検証する。
#define KU_CHECK(cond)                                          \
	do {                                                        \
		++::kutest::CheckCount();                               \
		if (!(cond)) ::kutest::ReportFail(__FILE__, __LINE__, #cond); \
	} while (0)

// 浮動小数が誤差 eps 以内で一致することを検証する。
#define KU_CHECK_NEAR(a, b, eps)                                       \
	do {                                                               \
		++::kutest::CheckCount();                                      \
		if (std::fabs((a) - (b)) > (eps))                              \
			::kutest::ReportFail(__FILE__, __LINE__, #a " ~= " #b);    \
	} while (0)
