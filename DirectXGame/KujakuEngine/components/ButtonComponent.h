#pragma once

#include "../math/Vector4.h"
#include "../scene/Component.h"
#include <array>
#include <string>

namespace KujakuEngine {

/// <summary>
/// UIのボタン(UnityのButton相当)。同じGameObjectのImageをtarget graphicとし、
/// ホバー/押下で色を変え、クリック時に名前付きイベント(UIEventBus)を発火する。
/// </summary>
class ButtonComponent : public Component {
public:
	enum class VisualState {
		Normal,
		Highlighted,
		Pressed,
		Disabled,
	};

	const char* GetTypeName() const override { return "Button"; }
	bool AllowMultiple() const override { return false; }

	void DrawInspector() override;
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;
	void OnAfterReadJson() override;

	bool IsInteractable() const { return interactable_; }

	/// <summary>現在の見た目状態に応じてtarget graphic(Image)の色を設定する。</summary>
	void ApplyVisualState(VisualState state);

	/// <summary>クリック確定時。onClickイベントを発火する。</summary>
	void FireOnClick();

private:
	void SyncEventBuffer();

	Vector4 normalColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	Vector4 highlightedColor_ = {0.90f, 0.90f, 0.90f, 1.0f};
	Vector4 pressedColor_ = {0.70f, 0.70f, 0.70f, 1.0f};
	Vector4 disabledColor_ = {0.50f, 0.50f, 0.50f, 0.6f};
	bool interactable_ = true;
	std::string onClickEvent_;

	std::array<char, 128> eventBuffer_{};
	bool eventBufferSynced_ = false;
};

} // namespace KujakuEngine
