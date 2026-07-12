#include "SelectionProvider.h"

namespace KujakuEngine {

namespace {

// プロバイダ未設定時のフォールバック。選択なし(nullptr)を返す。
class NullSelectionProvider : public ISelectionProvider {
public:
	GameObject* GetSelectedGameObject() const override { return nullptr; }
};

ISelectionProvider* g_provider = nullptr;

} // namespace

ISelectionProvider& GetSelectionProvider() {
	if (g_provider) {
		return *g_provider;
	}
	static NullSelectionProvider fallback;
	return fallback;
}

void SetSelectionProvider(ISelectionProvider* provider) { g_provider = provider; }

} // namespace KujakuEngine
