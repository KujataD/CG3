#pragma once

#include "../assets/AnimationClipAsset.h"
#include <array>
#include <string>
#include <vector>

namespace KujakuEngine {

class AnimatorComponent;
class GameObject;
class Scene;

/// <summary>
/// Unity風のAnimationウィンドウ。選択GameObjectのAnimatorのクリップを
/// ドープシート(トラック×キーフレーム)で編集し、スクラブプレビューする。
/// </summary>
class AnimationWindow {
public:
	void Draw(bool* pOpen);

	/// <summary>
	/// ウィンドウ非表示時にImGuiManagerから呼ばれ、プレビュー/録画状態を安全に解除します。
	/// </summary>
	void NotifyHidden();

private:
	/// <summary>
	/// "ComponentTypeName/チャンネル名" 形式のアニメーション可能チャンネル。
	/// </summary>
	struct NamedChannel {
		std::string path;
		float* value = nullptr;
	};

	void CollectChannels(GameObject& owner, const AnimatorComponent& animator, std::vector<NamedChannel>& outChannels) const;
	float* FindChannelValue(const std::vector<NamedChannel>& channels, const std::string& path) const;

	void BeginPreview(Scene& scene, AnimatorComponent& animator);
	void EndPreview(Scene& scene);
	void UpdatePreview(AnimatorComponent& animator);

	void DrawClipBar(AnimatorComponent& animator);

	/// <summary>
	/// 直前のアイテムをクリップアセットのドロップターゲットにします。
	/// </summary>
	void AcceptClipDrop(AnimatorComponent& animator);
	void DrawToolbar(Scene& scene, AnimatorComponent& animator, const std::vector<NamedChannel>& channels);
	void DrawTrackList(AnimatorComponent& animator, const std::vector<NamedChannel>& channels);
	void DrawDopeSheet(AnimatorComponent& animator, const std::vector<NamedChannel>& channels);
	void DrawCurveEditor(AnimatorComponent& animator, const std::vector<NamedChannel>& channels);
	void DrawClipCreation(AnimatorComponent& animator);

	/// <summary>
	/// キー右クリックメニュー共通のイージングプリセット項目(Smooth/Linear/Flat/Constant)。
	/// </summary>
	void DrawKeyPresetMenuItems(AnimationCurve& curve, int keyIndex);

	/// <summary>
	/// 指定トラックへ現在のチャンネル値でキーを追加し、前後キーのタンジェントをSmoothに整えます。
	/// </summary>
	void AddKeyAtTime(AnimatorComponent& animator, const std::string& trackPath, const std::vector<NamedChannel>& channels, float time);

	bool previewing_ = false;
	bool previewPlaying_ = false;
	bool previewReverse_ = false; // PingPong再生の往復方向
	bool recording_ = false;

	bool hasCopiedKey_ = false;
	AnimationKeyframe copiedKey_{};
	float previewTime_ = 0.0f;
	std::string previewSnapshotJson_;
	std::string previewOwnerInstanceId_;

	int selectedTrackIndex_ = -1;
	int selectedKeyIndex_ = -1;
	int viewMode_ = 0; // 0=Dopesheet, 1=Curves
	std::array<char, 128> newClipNameBuffer_{};
	std::string statusMessage_;
};

} // namespace KujakuEngine
