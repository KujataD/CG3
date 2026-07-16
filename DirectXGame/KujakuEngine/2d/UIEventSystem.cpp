#include "UIEventSystem.h"

#include "../components/ButtonComponent.h"
#include "../components/CanvasComponent.h"
#include "../components/RectTransformComponent.h"
#include "../runtime/UIInput.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include "UIRect.h"
#include <memory>

namespace KujakuEngine {
namespace {

// 押下開始したButton(離すまで保持)。Scene破棄で無効化されるためClearで消す。
ButtonComponent* gPressedButton = nullptr;

// ノードを再帰処理: すべてのButtonをNormalへ戻しつつ、ポインタ下の最前面Button(描画順で最後)を探す。
void ProcessNode(GameObject* node, const UIRect& parentRect, float pointerCanvasX, float pointerCanvasY, ButtonComponent*& outHit) {
	if (!node || !node->IsActive()) {
		return;
	}
	RectTransformComponent* rectTransform = node->GetComponent<RectTransformComponent>();
	if (!rectTransform) {
		return;
	}
	const UIRect rect = rectTransform->ComputeRect(parentRect);

	ButtonComponent* button = node->GetComponent<ButtonComponent>();
	if (button && button->IsEnabled()) {
		button->ApplyVisualState(ButtonComponent::VisualState::Normal); // 静止状態へ戻す
		if (rect.Contains(pointerCanvasX, pointerCanvasY)) {
			outHit = button; // 後勝ち = 最前面
		}
	}

	for (GameObject* child : node->GetChildren()) {
		ProcessNode(child, rect, pointerCanvasX, pointerCanvasY, outHit);
	}
}

} // namespace

void UpdateUIEventSystem(Scene& scene, float targetWidth, float targetHeight) {
	if (targetWidth <= 0.0f || targetHeight <= 0.0f) {
		return;
	}
	const UIPointerState& pointer = GetUIPointer();

	ButtonComponent* hit = nullptr;
	for (const std::unique_ptr<GameObject>& gameObject : scene.GetGameObjects()) {
		if (!gameObject || !gameObject->IsActiveInHierarchy()) {
			continue;
		}
		CanvasComponent* canvas = gameObject->GetComponent<CanvasComponent>();
		if (!canvas || !canvas->IsEnabled()) {
			continue;
		}
		const CanvasComponent::Layout layout = canvas->GetLayout(targetWidth, targetHeight);
		const UIRect rootRect{0.0f, 0.0f, layout.canvasWidth, layout.canvasHeight};
		const float pointerCanvasX = pointer.x / layout.scaleFactor;
		const float pointerCanvasY = pointer.y / layout.scaleFactor;
		for (GameObject* child : gameObject->GetChildren()) {
			ProcessNode(child, rootRect, pointerCanvasX, pointerCanvasY, hit);
		}
	}

	if (!pointer.inside) {
		hit = nullptr;
	}

	// 押下開始
	if (hit && hit->IsInteractable() && pointer.pressed) {
		gPressedButton = hit;
	}

	// ホバー/押下の見た目(ProcessNodeで全ButtonはNormal済みなので、対象だけ上書き)。
	if (hit && hit->IsInteractable()) {
		if (gPressedButton == hit && pointer.held) {
			hit->ApplyVisualState(ButtonComponent::VisualState::Pressed);
		} else {
			hit->ApplyVisualState(ButtonComponent::VisualState::Highlighted);
		}
	}

	// 離した瞬間: 押下開始と同じButton上ならクリック確定。
	if (pointer.released) {
		if (gPressedButton && gPressedButton == hit) {
			gPressedButton->FireOnClick();
		}
		gPressedButton = nullptr;
	}
}

} // namespace KujakuEngine
