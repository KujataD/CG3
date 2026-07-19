#include "AnimationWindow.h"

#ifdef USE_IMGUI
#include "../../externals/imgui/imgui.h"
#include "../assets/AnimationClipAsset.h"
#include "../base/ProjectPath.h"
#include "../components/AnimatorComponent.h"
#include "../runtime/AnimationRecordingState.h"
#include "../runtime/PlayState.h"
#include "../scene/ComponentFactory.h"
#include "../scene/GameObject.h"
#include "../scene/Scene.h"
#include "AssetDatabase.h"
#include "EditorApplication.h"
#include "EditorImGuiUtil.h"
#include "EditorSelection.h"
#include "SceneJsonImporter.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#endif // USE_IMGUI

namespace KujakuEngine {

#ifdef USE_IMGUI

namespace {

constexpr float kTrackListWidth = 230.0f;
constexpr float kRulerHeight = 24.0f;
constexpr float kRowHeight = 22.0f;
constexpr float kKeyHalfSize = 5.0f;
constexpr float kTimeSnap = 0.01f;

// 選択GameObjectのAnimatorComponentを返す(無ければnullptr)。
AnimatorComponent* FindSelectedAnimator(GameObject*& outOwner) {
	outOwner = EditorSelection::GetInstance()->GetSelectedGameObject();
	if (!outOwner) {
		return nullptr;
	}
	for (const std::unique_ptr<Component>& component : outOwner->GetComponents()) {
		if (AnimatorComponent* animator = dynamic_cast<AnimatorComponent*>(component.get())) {
			return animator;
		}
	}
	return nullptr;
}

float SnapTime(float time) {
	return std::round(time / kTimeSnap) * kTimeSnap;
}

// ドープシートの時間表示レンジ(秒)。クリップより少し広く、最低1秒。
float ComputeViewDuration(const AnimationClipData& clip) {
	return (std::max)(clip.GetDuration() * 1.15f, 1.0f);
}

ImU32 KeyColor(bool selected) {
	return selected ? IM_COL32(255, 200, 60, 255) : IM_COL32(160, 200, 255, 255);
}

void DrawKeyDiamond(ImDrawList* drawList, const ImVec2& center, ImU32 color) {
	const float half = kKeyHalfSize;
	ImVec2 points[4] = {
	    {center.x, center.y - half},
	    {center.x + half, center.y},
	    {center.x, center.y + half},
	    {center.x - half, center.y},
	};
	drawList->AddConvexPolyFilled(points, 4, color);
	drawList->AddPolyline(points, 4, IM_COL32(30, 30, 30, 255), ImDrawFlags_Closed, 1.0f);
}

} // namespace

void AnimationWindow::CollectChannels(GameObject& owner, const AnimatorComponent& animator, std::vector<NamedChannel>& outChannels) const {
	std::vector<AnimatableChannel> channels;
	for (const std::unique_ptr<Component>& component : owner.GetComponents()) {
		if (!component || component.get() == &animator) {
			continue;
		}
		channels.clear();
		component->CollectAnimatableChannels(channels);
		std::string prefix = std::string(component->GetTypeName()) + "/";
		for (const AnimatableChannel& channel : channels) {
			outChannels.push_back({prefix + channel.path, channel.value});
		}
	}
}

float* AnimationWindow::FindChannelValue(const std::vector<NamedChannel>& channels, const std::string& path) const {
	for (const NamedChannel& channel : channels) {
		if (channel.path == path) {
			return channel.value;
		}
	}
	return nullptr;
}

void AnimationWindow::BeginPreview(Scene& scene, AnimatorComponent& animator) {
	if (previewing_) {
		return;
	}
	previewSnapshotJson_ = scene.ToJson();
	GameObject* owner = animator.GetOwner();
	previewOwnerInstanceId_ = owner ? owner->GetInstanceId() : std::string();
	previewing_ = true;
	previewPlaying_ = false;
	animator.ResolveBindings();
	animator.EvaluateAt(previewTime_);
}

void AnimationWindow::EndPreview(Scene& scene) {
	if (!previewing_) {
		return;
	}
	previewing_ = false;
	previewPlaying_ = false;
	// 録画はプレビュー前提なので一緒に終了する。
	recording_ = false;
	SetAnimationRecording(false);

	// シーン復元でAnimatorも再構築される(クリップはファイルから再読込される)ため、
	// プレビュー中の未保存のクリップ編集を退避して復元後に引き継ぐ。
	AnimationClipData editedClip;
	std::string editedClipPath;
	bool hasEditedClip = false;
	{
		GameObject* owner = nullptr;
		AnimatorComponent* animator = FindSelectedAnimator(owner);
		if (animator && animator->HasClip() && owner && owner->GetInstanceId() == previewOwnerInstanceId_) {
			editedClip = animator->GetClip();
			editedClipPath = animator->GetClipPath();
			hasEditedClip = true;
		}
	}

	if (!previewSnapshotJson_.empty()) {
		// プレビューで書き換えた値をスナップショットへ巻き戻す。
		SceneJsonImporter::ApplySceneJsonString(scene, previewSnapshotJson_);
		scene.UpdateWorldTransforms();
		previewSnapshotJson_.clear();
	}

	if (hasEditedClip) {
		GameObject* restoredOwner = scene.FindGameObjectByInstanceId(previewOwnerInstanceId_);
		if (restoredOwner) {
			for (const std::unique_ptr<Component>& component : restoredOwner->GetComponents()) {
				AnimatorComponent* restoredAnimator = dynamic_cast<AnimatorComponent*>(component.get());
				if (restoredAnimator && restoredAnimator->GetClipPath() == editedClipPath) {
					restoredAnimator->GetClip() = std::move(editedClip);
					restoredAnimator->ResolveBindings();
					break;
				}
			}
		}
	}

	previewOwnerInstanceId_.clear();
}

void AnimationWindow::UpdatePreview(AnimatorComponent& animator) {
	if (!previewing_) {
		return;
	}

	float duration = animator.GetClip().GetDuration();
	if (previewPlaying_ && duration > 0.0f) {
		float deltaTime = ImGui::GetIO().DeltaTime;
		switch (animator.GetClip().wrapMode) {
		case AnimationWrapMode::Once:
			previewTime_ += deltaTime;
			if (previewTime_ >= duration) {
				previewTime_ = duration;
				previewPlaying_ = false;
			}
			break;
		case AnimationWrapMode::Loop:
			previewTime_ = std::fmod(previewTime_ + deltaTime, duration);
			break;
		case AnimationWrapMode::PingPong:
			if (previewReverse_) {
				previewTime_ -= deltaTime;
				if (previewTime_ <= 0.0f) {
					previewTime_ = -previewTime_;
					previewReverse_ = false;
				}
			} else {
				previewTime_ += deltaTime;
				if (previewTime_ >= duration) {
					previewTime_ = duration - (previewTime_ - duration);
					previewReverse_ = true;
				}
			}
			previewTime_ = std::clamp(previewTime_, 0.0f, duration);
			break;
		}
	}

	animator.EvaluateAt(previewTime_);
}

void AnimationWindow::AddKeyAtTime(AnimatorComponent& animator, const std::string& trackPath, const std::vector<NamedChannel>& channels, float time) {
	AnimationTrack& track = animator.GetClip().GetOrAddTrack(trackPath);

	AnimationKeyframe key;
	key.time = (std::max)(SnapTime(time), 0.0f);
	float* channelValue = FindChannelValue(channels, trackPath);
	key.value = channelValue ? *channelValue : track.curve.Evaluate(key.time);

	int keyIndex = track.curve.AddKey(key);
	// 追加キーと前後キーのタンジェントを滑らかに整える。
	AnimationClipAsset::SetSmoothTangents(track.curve, keyIndex - 1);
	AnimationClipAsset::SetSmoothTangents(track.curve, keyIndex);
	AnimationClipAsset::SetSmoothTangents(track.curve, keyIndex + 1);
}

void AnimationWindow::DrawKeyPresetMenuItems(AnimationCurve& curve, int keyIndex) {
	if (keyIndex < 0 || keyIndex >= static_cast<int>(curve.keys.size())) {
		return;
	}
	AnimationKeyframe& key = curve.keys[keyIndex];

	// --- タンジェント系プリセット(イージング指定は解除してタンジェント補間に戻す) ---
	if (ImGui::MenuItem("Smooth")) {
		key.easing = EasingType::None;
		AnimationClipAsset::SetSmoothTangents(curve, keyIndex);
	}
	if (ImGui::MenuItem("Linear")) {
		key.easing = EasingType::None;
		AnimationClipAsset::SetLinearTangents(curve, keyIndex);
	}
	if (ImGui::MenuItem("Flat")) {
		key.easing = EasingType::None;
		key.inTangent = 0.0f;
		key.outTangent = 0.0f;
	}
	if (ImGui::MenuItem("Constant")) {
		key.easing = EasingType::None;
		key.inTangent = AnimationClipAsset::ConstantTangent();
		key.outTangent = AnimationClipAsset::ConstantTangent();
	}

	// --- easings.netのイージングテンプレ(このキー→次のキーの区間へ適用) ---
	ImGui::Separator();
	if (ImGui::BeginMenu("Easing (to Next Key)")) {
		struct EasingFamily {
			const char* name;
			EasingType easeIn;
			EasingType easeOut;
			EasingType easeInOut;
		};
		static constexpr EasingFamily kFamilies[] = {
		    {"Sine", EasingType::EaseInSine, EasingType::EaseOutSine, EasingType::EaseInOutSine},
		    {"Quad", EasingType::EaseInQuad, EasingType::EaseOutQuad, EasingType::EaseInOutQuad},
		    {"Cubic", EasingType::EaseInCubic, EasingType::EaseOutCubic, EasingType::EaseInOutCubic},
		    {"Quart", EasingType::EaseInQuart, EasingType::EaseOutQuart, EasingType::EaseInOutQuart},
		    {"Quint", EasingType::EaseInQuint, EasingType::EaseOutQuint, EasingType::EaseInOutQuint},
		    {"Expo", EasingType::EaseInExpo, EasingType::EaseOutExpo, EasingType::EaseInOutExpo},
		    {"Circ", EasingType::EaseInCirc, EasingType::EaseOutCirc, EasingType::EaseInOutCirc},
		    {"Back", EasingType::EaseInBack, EasingType::EaseOutBack, EasingType::EaseInOutBack},
		    {"Elastic", EasingType::EaseInElastic, EasingType::EaseOutElastic, EasingType::EaseInOutElastic},
		    {"Bounce", EasingType::EaseInBounce, EasingType::EaseOutBounce, EasingType::EaseInOutBounce},
		};

		if (ImGui::MenuItem("Tangent (Free)", nullptr, key.easing == EasingType::None)) {
			key.easing = EasingType::None;
		}
		ImGui::Separator();
		for (const EasingFamily& family : kFamilies) {
			if (ImGui::BeginMenu(family.name)) {
				if (ImGui::MenuItem("Ease In", nullptr, key.easing == family.easeIn)) {
					key.easing = family.easeIn;
				}
				if (ImGui::MenuItem("Ease Out", nullptr, key.easing == family.easeOut)) {
					key.easing = family.easeOut;
				}
				if (ImGui::MenuItem("Ease In Out", nullptr, key.easing == family.easeInOut)) {
					key.easing = family.easeInOut;
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenu();
	}
}

void AnimationWindow::DrawClipCreation(AnimatorComponent& animator) {
	ImGui::TextDisabled("No animation clip. Create one:");
	ImGui::InputText("Clip Name", newClipNameBuffer_.data(), newClipNameBuffer_.size());
	ImGui::SameLine();
	if (ImGui::Button("Create")) {
		std::string clipName = newClipNameBuffer_.data();
		if (clipName.empty()) {
			clipName = "NewAnimation";
		}

		std::filesystem::path projectRoot = DetectEditorProjectRoot();
		std::filesystem::path animationsDirectory = projectRoot / "Animations";
		std::error_code errorCode;
		std::filesystem::create_directories(animationsDirectory, errorCode);

		std::filesystem::path clipPath = animationsDirectory / (clipName + ".anim.json");
		std::string message;
		if (AnimationClipAsset::CreateDefaultFile(clipPath, message)) {
			AssetDatabase::GetInstance().GetOrCreateAssetId(clipPath);
			animator.SetClipPath(clipPath.generic_string());
			statusMessage_ = "Created: " + clipPath.filename().generic_string();
			ImGui::CloseCurrentPopup();
		} else {
			statusMessage_ = message;
		}
	}
}

void AnimationWindow::AcceptClipDrop(AnimatorComponent& animator) {
	if (!ImGui::BeginDragDropTarget()) {
		return;
	}
	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kProjectAnimClipDragPayloadType);
	if (payload && payload->DataSize > 0) {
		animator.SetClipPath(static_cast<const char*>(payload->Data));
		selectedTrackIndex_ = -1;
		selectedKeyIndex_ = -1;
		previewTime_ = 0.0f;
		previewReverse_ = false;
		statusMessage_ = "Clip added.";
	}
	ImGui::EndDragDropTarget();
}

void AnimationWindow::DrawClipBar(AnimatorComponent& animator) {
	const std::vector<AnimationClipReference>& references = animator.GetClipReferences();

	std::string currentLabel;
	if (animator.HasClip()) {
		currentLabel = animator.GetClip().name + "  (" + animator.GetClipPath() + ")";
	} else if (references.empty()) {
		currentLabel = "(no clip)";
	} else {
		currentLabel = animator.GetClipPath();
	}

	ImGui::SetNextItemWidth(340.0f);
	if (ImGui::BeginCombo("##ClipSelector", currentLabel.c_str())) {
		for (int index = 0; index < static_cast<int>(references.size()); ++index) {
			bool isSelected = (index == animator.GetCurrentClipIndex());
			std::string itemLabel = references[index].path + "##clip" + std::to_string(index);
			if (ImGui::Selectable(itemLabel.c_str(), isSelected) && !isSelected) {
				// 切替時、未保存のクリップ編集はファイルから再読込されて破棄される。
				animator.SetCurrentClipIndex(index);
				selectedTrackIndex_ = -1;
				selectedKeyIndex_ = -1;
				previewTime_ = 0.0f;
				previewReverse_ = false;
				statusMessage_ = "Switched clip (unsaved edits discarded).";
			}
		}
		ImGui::EndCombo();
	}
	AcceptClipDrop(animator);

	ImGui::SameLine();
	if (ImGui::Button("+##NewClip")) {
		ImGui::OpenPopup("NewClipPopup");
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("新しいAnimation Clipを作成して追加");
	}
	if (ImGui::BeginPopup("NewClipPopup")) {
		DrawClipCreation(animator);
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	ImGui::TextDisabled("(Projectから.anim.jsonをD&Dで追加)");
}

void AnimationWindow::DrawToolbar(Scene& scene, AnimatorComponent& animator, const std::vector<NamedChannel>& channels) {
	// --- 表示切替(Unityのドープシート/カーブ切替に相当) ---
	if (ImGui::RadioButton("Dopesheet", viewMode_ == 0)) {
		viewMode_ = 0;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Curves", viewMode_ == 1)) {
		viewMode_ = 1;
	}
	ImGui::SameLine();
	ImGui::TextDisabled("|");
	ImGui::SameLine();

	// --- 録画(Unityの赤丸)。録画ONはプレビューも自動でONにする ---
	bool wasRecording = recording_;
	if (wasRecording) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.18f, 0.18f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
	}
	if (ImGui::Button("[Rec]")) {
		if (recording_) {
			recording_ = false;
			SetAnimationRecording(false);
		} else {
			if (!previewing_) {
				BeginPreview(scene, animator);
			}
			recording_ = true;
			SetAnimationRecording(true);
		}
	}
	if (wasRecording) {
		ImGui::PopStyleColor(3);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("録画モード: Inspectorで変更した値をプレイヘッドへ自動キー登録");
	}
	ImGui::SameLine();

	// --- プレビュー制御 ---
	bool previewToggled = previewing_;
	if (ImGui::Checkbox("Preview", &previewToggled)) {
		if (previewToggled) {
			BeginPreview(scene, animator);
		} else {
			EndPreview(scene);
		}
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(!previewing_);
	if (previewPlaying_) {
		if (ImGui::Button("Stop##PreviewPlay")) {
			previewPlaying_ = false;
		}
	} else {
		if (ImGui::Button("Play##PreviewPlay")) {
			previewPlaying_ = true;
			previewReverse_ = false;
			if (previewTime_ >= animator.GetClip().GetDuration()) {
				previewTime_ = 0.0f;
			}
		}
	}
	ImGui::EndDisabled();

	// --- 時刻 ---
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90.0f);
	float timeValue = previewTime_;
	if (ImGui::DragFloat("##Time", &timeValue, 0.01f, 0.0f, 0.0f, "%.2fs")) {
		previewTime_ = (std::max)(timeValue, 0.0f);
	}

	ImGui::SameLine();
	ImGui::Text("/ %.2fs", animator.GetClip().GetDuration());

	// --- ラップモード・キー追加・保存 ---
	ImGui::SameLine();
	const char* wrapItems[] = {"Once", "Loop", "PingPong"};
	int wrapIndex = static_cast<int>(animator.GetClip().wrapMode);
	ImGui::SetNextItemWidth(95.0f);
	if (ImGui::Combo("##WrapMode", &wrapIndex, wrapItems, 3)) {
		animator.GetClip().wrapMode = static_cast<AnimationWrapMode>(wrapIndex);
		previewReverse_ = false;
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(animator.GetClip().tracks.empty());
	if (ImGui::Button("Key All")) {
		for (const AnimationTrack& track : animator.GetClip().tracks) {
			AddKeyAtTime(animator, track.path, channels, previewTime_);
		}
	}
	ImGui::EndDisabled();

	ImGui::SameLine();
	if (ImGui::Button("Save Clip")) {
		std::string message;
		if (animator.SaveClip(message)) {
			statusMessage_ = "Saved: " + animator.GetClipPath();
		} else {
			statusMessage_ = message;
		}
	}

	if (!statusMessage_.empty()) {
		ImGui::SameLine();
		ImGui::TextDisabled("%s", statusMessage_.c_str());
	}
}

void AnimationWindow::DrawTrackList(AnimatorComponent& animator, const std::vector<NamedChannel>& channels) {
	AnimationClipData& clip = animator.GetClip();

	// トラック追加ポップアップ(まだトラックが無いチャンネルを列挙)。
	if (ImGui::Button("Add Track...", ImVec2(-1.0f, 0.0f))) {
		ImGui::OpenPopup("AddTrackPopup");
	}
	if (ImGui::BeginPopup("AddTrackPopup")) {
		bool anyCandidate = false;
		for (const NamedChannel& channel : channels) {
			if (clip.FindTrack(channel.path)) {
				continue;
			}
			anyCandidate = true;
			if (ImGui::MenuItem(channel.path.c_str())) {
				AddKeyAtTime(animator, channel.path, channels, previewTime_);
				selectedTrackIndex_ = static_cast<int>(clip.tracks.size()) - 1;
				ImGui::CloseCurrentPopup();
			}
		}
		if (!anyCandidate) {
			ImGui::TextDisabled("No more channels.");
		}
		ImGui::EndPopup();
	}

	// トラック一覧。
	int removeTrackIndex = -1;
	for (int trackIndex = 0; trackIndex < static_cast<int>(clip.tracks.size()); ++trackIndex) {
		const AnimationTrack& track = clip.tracks[trackIndex];
		ImGui::PushID(trackIndex);

		bool isSelected = (selectedTrackIndex_ == trackIndex);
		bool isBound = FindChannelValue(channels, track.path) != nullptr;
		if (!isBound) {
			// 解決できないトラック(Component欠落など)は赤字で警告表示。
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
		}
		if (ImGui::Selectable(track.path.c_str(), isSelected, 0, ImVec2(0.0f, kRowHeight - 4.0f))) {
			if (selectedTrackIndex_ != trackIndex) {
				selectedKeyIndex_ = -1;
			}
			selectedTrackIndex_ = trackIndex;
		}
		if (!isBound) {
			ImGui::PopStyleColor();
		}

		if (ImGui::BeginPopupContextItem("TrackContext")) {
			if (ImGui::MenuItem("Add Key At Playhead")) {
				// channelsはconst参照でAddKeyAtTimeが解決する。
				AddKeyAtTime(animator, track.path, channels, previewTime_);
			}
			if (ImGui::MenuItem("Paste Key At Playhead", nullptr, false, hasCopiedKey_)) {
				// コピー元の値/タンジェント/イージングを保ったままプレイヘッド時刻へ貼り付ける。
				AnimationKeyframe pastedKey = copiedKey_;
				pastedKey.time = (std::max)(SnapTime(previewTime_), 0.0f);
				clip.tracks[trackIndex].curve.AddKey(pastedKey);
			}
			if (ImGui::MenuItem("Delete Track")) {
				removeTrackIndex = trackIndex;
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	if (removeTrackIndex >= 0) {
		clip.tracks.erase(clip.tracks.begin() + removeTrackIndex);
		if (selectedTrackIndex_ >= static_cast<int>(clip.tracks.size())) {
			selectedTrackIndex_ = static_cast<int>(clip.tracks.size()) - 1;
		}
	}
}

void AnimationWindow::DrawDopeSheet(AnimatorComponent& animator, const std::vector<NamedChannel>& channels) {
	AnimationClipData& clip = animator.GetClip();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	canvasSize.x = (std::max)(canvasSize.x, 50.0f);
	canvasSize.y = (std::max)(canvasSize.y, kRulerHeight + kRowHeight);

	const float viewDuration = ComputeViewDuration(clip);
	const float pixelsPerSecond = (canvasSize.x - 12.0f) / viewDuration;
	auto timeToX = [&](float time) { return canvasPosition.x + 6.0f + time * pixelsPerSecond; };
	auto xToTime = [&](float x) { return (x - canvasPosition.x - 6.0f) / pixelsPerSecond; };

	// 背景。
	drawList->AddRectFilled(canvasPosition, ImVec2(canvasPosition.x + canvasSize.x, canvasPosition.y + canvasSize.y), IM_COL32(28, 28, 30, 255));

	// --- 目盛り(0.1s間隔の細線 + 0.5s間隔のラベル付き太線) ---
	for (float tick = 0.0f; tick <= viewDuration + 1.0e-4f; tick += 0.1f) {
		bool isMajor = (std::abs(std::fmod(tick + 1.0e-4f, 0.5f)) < 1.0e-3f);
		float x = timeToX(tick);
		ImU32 lineColor = isMajor ? IM_COL32(110, 110, 115, 255) : IM_COL32(60, 60, 64, 255);
		float lineTop = canvasPosition.y + (isMajor ? 4.0f : 12.0f);
		drawList->AddLine(ImVec2(x, lineTop), ImVec2(x, canvasPosition.y + canvasSize.y), lineColor, 1.0f);
		if (isMajor) {
			char label[16];
			snprintf(label, sizeof(label), "%.1f", tick);
			drawList->AddText(ImVec2(x + 3.0f, canvasPosition.y + 2.0f), IM_COL32(170, 170, 175, 255), label);
		}
	}

	// --- ルーラーでのスクラブ(ドラッグで時刻変更) ---
	ImGui::SetCursorScreenPos(canvasPosition);
	ImGui::InvisibleButton("##DopeSheetRuler", ImVec2(canvasSize.x, kRulerHeight));
	if (ImGui::IsItemActive()) {
		previewTime_ = (std::max)(SnapTime(xToTime(ImGui::GetMousePos().x)), 0.0f);
		previewPlaying_ = false;
	}

	// --- 各トラック行とキー ---
	int trackCount = static_cast<int>(clip.tracks.size());
	for (int trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
		AnimationTrack& track = clip.tracks[trackIndex];
		float rowTop = canvasPosition.y + kRulerHeight + trackIndex * kRowHeight;
		float rowCenterY = rowTop + kRowHeight * 0.5f;

		// 行の背景(選択中は明るく)。
		ImU32 rowColor = (trackIndex == selectedTrackIndex_) ? IM_COL32(52, 60, 74, 255) : ((trackIndex % 2 == 0) ? IM_COL32(36, 36, 40, 255) : IM_COL32(32, 32, 36, 255));
		drawList->AddRectFilled(ImVec2(canvasPosition.x, rowTop), ImVec2(canvasPosition.x + canvasSize.x, rowTop + kRowHeight), rowColor);

		// 行のダブルクリックでその時刻へキー追加。
		// 後からsubmitするキーのボタンがクリックを受け取れるよう、行ボタンはオーバーラップを許可する
		// (これが無いと先にsubmitされた行がクリックを奪い、キーをドラッグできない)。
		ImGui::SetCursorScreenPos(ImVec2(canvasPosition.x, rowTop));
		ImGui::PushID(trackIndex);
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("##Row", ImVec2(canvasSize.x, kRowHeight));
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			selectedTrackIndex_ = trackIndex;
			AddKeyAtTime(animator, track.path, channels, xToTime(ImGui::GetMousePos().x));
		}
		if (ImGui::IsItemClicked()) {
			if (selectedTrackIndex_ != trackIndex) {
				selectedKeyIndex_ = -1;
			}
			selectedTrackIndex_ = trackIndex;
		}

		// キー(ひし形)。ドラッグで時刻変更(前後キーの間にクランプ)、右クリックで削除。
		int removeKeyIndex = -1;
		for (int keyIndex = 0; keyIndex < static_cast<int>(track.curve.keys.size()); ++keyIndex) {
			AnimationKeyframe& key = track.curve.keys[keyIndex];
			ImVec2 keyCenter(timeToX(key.time), rowCenterY);

			ImGui::PushID(keyIndex);
			ImGui::SetCursorScreenPos(ImVec2(keyCenter.x - kKeyHalfSize - 2.0f, keyCenter.y - kKeyHalfSize - 2.0f));
			ImGui::InvisibleButton("##Key", ImVec2((kKeyHalfSize + 2.0f) * 2.0f, (kKeyHalfSize + 2.0f) * 2.0f));

			bool keyActive = ImGui::IsItemActive();
			if (keyActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
				float newTime = SnapTime(xToTime(ImGui::GetMousePos().x));
				// 並び順を保つため前後キーの間へクランプする。
				float minTime = (keyIndex > 0) ? track.curve.keys[keyIndex - 1].time + kTimeSnap : 0.0f;
				float maxTime = (keyIndex + 1 < static_cast<int>(track.curve.keys.size())) ? track.curve.keys[keyIndex + 1].time - kTimeSnap : 1.0e6f;
				key.time = std::clamp(newTime, minTime, maxTime);
			}

			if (ImGui::BeginPopupContextItem("KeyContext")) {
				if (ImGui::MenuItem("Copy Key")) {
					copiedKey_ = key;
					hasCopiedKey_ = true;
				}
				ImGui::Separator();
				DrawKeyPresetMenuItems(track.curve, keyIndex);
				ImGui::Separator();
				if (ImGui::MenuItem("Delete Key")) {
					removeKeyIndex = keyIndex;
				}
				ImGui::EndPopup();
			}

			DrawKeyDiamond(drawList, keyCenter, KeyColor(keyActive || trackIndex == selectedTrackIndex_));
			ImGui::PopID();
		}

		if (removeKeyIndex >= 0) {
			track.curve.keys.erase(track.curve.keys.begin() + removeKeyIndex);
		}

		ImGui::PopID();
	}

	// --- プレイヘッド(最前面) ---
	float playheadX = timeToX(previewTime_);
	drawList->AddLine(ImVec2(playheadX, canvasPosition.y), ImVec2(playheadX, canvasPosition.y + canvasSize.y), IM_COL32(255, 90, 90, 255), 2.0f);
	drawList->AddTriangleFilled(ImVec2(playheadX - 5.0f, canvasPosition.y), ImVec2(playheadX + 5.0f, canvasPosition.y), ImVec2(playheadX, canvasPosition.y + 8.0f), IM_COL32(255, 90, 90, 255));

	// キャンバス領域をアイテムとして確保する。
	// SetCursorScreenPosだけで境界を広げるとimguiのアサート(SetCursorPos to extend boundaries)になるため、
	// 必ずDummyを最後に置いてウィンドウ境界を正しく成長させる。
	ImGui::SetCursorScreenPos(canvasPosition);
	ImGui::Dummy(canvasSize);
	AcceptClipDrop(animator);
}

void AnimationWindow::DrawCurveEditor(AnimatorComponent& animator, const std::vector<NamedChannel>& channels) {
	(void)channels;
	AnimationClipData& clip = animator.GetClip();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	canvasSize.x = (std::max)(canvasSize.x, 50.0f);
	canvasSize.y = (std::max)(canvasSize.y, kRulerHeight + 80.0f);

	drawList->AddRectFilled(canvasPosition, ImVec2(canvasPosition.x + canvasSize.x, canvasPosition.y + canvasSize.y), IM_COL32(24, 24, 26, 255));

	if (selectedTrackIndex_ < 0 || selectedTrackIndex_ >= static_cast<int>(clip.tracks.size())) {
		drawList->AddText(ImVec2(canvasPosition.x + 12.0f, canvasPosition.y + kRulerHeight + 8.0f), IM_COL32(150, 150, 155, 255), "Select a track to edit its curve.");
		ImGui::SetCursorScreenPos(canvasPosition);
		ImGui::Dummy(canvasSize);
		return;
	}

	AnimationTrack& track = clip.tracks[selectedTrackIndex_];
	AnimationCurve& curve = track.curve;

	const float viewDuration = ComputeViewDuration(clip);
	const float pixelsPerSecond = (canvasSize.x - 12.0f) / viewDuration;
	auto timeToX = [&](float time) { return canvasPosition.x + 6.0f + time * pixelsPerSecond; };
	auto xToTime = [&](float x) { return (x - canvasPosition.x - 6.0f) / pixelsPerSecond; };

	// --- 値レンジのオートフィット(キー値 + カーブのサンプル値から算出) ---
	float minValue = FLT_MAX;
	float maxValue = -FLT_MAX;
	for (const AnimationKeyframe& key : curve.keys) {
		minValue = (std::min)(minValue, key.value);
		maxValue = (std::max)(maxValue, key.value);
	}
	constexpr int kFitSampleCount = 64;
	for (int i = 0; i <= kFitSampleCount; ++i) {
		float sampled = curve.Evaluate(viewDuration * static_cast<float>(i) / kFitSampleCount);
		minValue = (std::min)(minValue, sampled);
		maxValue = (std::max)(maxValue, sampled);
	}
	if (curve.keys.empty()) {
		minValue = 0.0f;
		maxValue = 1.0f;
	}
	float valueRange = maxValue - minValue;
	if (valueRange < 1.0e-3f) {
		minValue -= 0.5f;
		maxValue += 0.5f;
	} else {
		minValue -= valueRange * 0.15f;
		maxValue += valueRange * 0.15f;
	}
	valueRange = maxValue - minValue;

	const float plotTop = canvasPosition.y + kRulerHeight;
	const float plotHeight = canvasSize.y - kRulerHeight - 4.0f;
	const float pixelsPerValue = plotHeight / valueRange;
	auto valueToY = [&](float value) { return plotTop + (maxValue - value) * pixelsPerValue; };

	// --- 時間目盛り(ドープシートと同じ) ---
	for (float tick = 0.0f; tick <= viewDuration + 1.0e-4f; tick += 0.1f) {
		bool isMajor = (std::abs(std::fmod(tick + 1.0e-4f, 0.5f)) < 1.0e-3f);
		float x = timeToX(tick);
		drawList->AddLine(ImVec2(x, canvasPosition.y + (isMajor ? 4.0f : 12.0f)), ImVec2(x, canvasPosition.y + canvasSize.y), isMajor ? IM_COL32(90, 90, 95, 255) : IM_COL32(50, 50, 54, 255), 1.0f);
		if (isMajor) {
			char label[16];
			snprintf(label, sizeof(label), "%.1f", tick);
			drawList->AddText(ImVec2(x + 3.0f, canvasPosition.y + 2.0f), IM_COL32(170, 170, 175, 255), label);
		}
	}

	// --- 値グリッド(1/2/5系列の切りの良い間隔) ---
	{
		float rawStep = valueRange / 5.0f;
		float magnitude = std::pow(10.0f, std::floor(std::log10((std::max)(rawStep, 1.0e-6f))));
		float normalized = rawStep / magnitude;
		float niceStep = (normalized < 1.5f ? 1.0f : normalized < 3.5f ? 2.0f : normalized < 7.5f ? 5.0f : 10.0f) * magnitude;
		for (float gridValue = std::ceil(minValue / niceStep) * niceStep; gridValue <= maxValue + 1.0e-6f; gridValue += niceStep) {
			float y = valueToY(gridValue);
			bool isZero = std::abs(gridValue) < niceStep * 0.5f;
			drawList->AddLine(ImVec2(canvasPosition.x, y), ImVec2(canvasPosition.x + canvasSize.x, y), isZero ? IM_COL32(110, 110, 90, 255) : IM_COL32(52, 52, 56, 255), 1.0f);
			char label[24];
			snprintf(label, sizeof(label), "%.3g", gridValue);
			drawList->AddText(ImVec2(canvasPosition.x + 4.0f, y - 14.0f), IM_COL32(140, 140, 145, 255), label);
		}
	}

	// --- ルーラーでのスクラブ ---
	ImGui::SetCursorScreenPos(canvasPosition);
	ImGui::InvisibleButton("##CurveRuler", ImVec2(canvasSize.x, kRulerHeight));
	if (ImGui::IsItemActive()) {
		previewTime_ = (std::max)(SnapTime(xToTime(ImGui::GetMousePos().x)), 0.0f);
		previewPlaying_ = false;
	}

	// --- プロット領域(ダブルクリックでキー追加 / クリックでキー選択解除) ---
	// カーブビューはイージング編集専用。値はカーブ上の現在値を使い、直接指定はさせない。
	ImGui::SetCursorScreenPos(ImVec2(canvasPosition.x, plotTop));
	ImGui::SetNextItemAllowOverlap();
	ImGui::InvisibleButton("##CurvePlot", ImVec2(canvasSize.x, plotHeight));
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		AnimationKeyframe key;
		key.time = (std::max)(SnapTime(xToTime(ImGui::GetMousePos().x)), 0.0f);
		key.value = curve.Evaluate(key.time);
		int keyIndex = curve.AddKey(key);
		AnimationClipAsset::SetSmoothTangents(curve, keyIndex - 1);
		AnimationClipAsset::SetSmoothTangents(curve, keyIndex);
		AnimationClipAsset::SetSmoothTangents(curve, keyIndex + 1);
		selectedKeyIndex_ = keyIndex;
	} else if (ImGui::IsItemClicked()) {
		selectedKeyIndex_ = -1;
	}

	// --- カーブ本体(2pxごとにサンプルして折れ線描画) ---
	if (!curve.keys.empty()) {
		const float plotLeft = canvasPosition.x + 2.0f;
		const float plotRight = canvasPosition.x + canvasSize.x - 2.0f;
		for (float x = plotLeft; x < plotRight; x += 2.0f) {
			drawList->PathLineTo(ImVec2(x, valueToY(curve.Evaluate(xToTime(x)))));
		}
		drawList->PathStroke(IM_COL32(120, 220, 140, 255), ImDrawFlags_None, 2.0f);
	}

	// --- キー(2Dドラッグ=時刻+値、右クリックでプリセット/削除) ---
	int removeKeyIndex = -1;
	for (int keyIndex = 0; keyIndex < static_cast<int>(curve.keys.size()); ++keyIndex) {
		AnimationKeyframe& key = curve.keys[keyIndex];
		ImVec2 keyCenter(timeToX(key.time), valueToY(key.value));

		ImGui::PushID(keyIndex);
		ImGui::SetCursorScreenPos(ImVec2(keyCenter.x - kKeyHalfSize - 2.0f, keyCenter.y - kKeyHalfSize - 2.0f));
		ImGui::InvisibleButton("##CurveKey", ImVec2((kKeyHalfSize + 2.0f) * 2.0f, (kKeyHalfSize + 2.0f) * 2.0f));

		bool keyActive = ImGui::IsItemActive();
		if (ImGui::IsItemClicked() || keyActive) {
			selectedKeyIndex_ = keyIndex;
		}
		// カーブビューでは値は変更させない(イージング編集専用)。ドラッグは時刻のみ。
		if (keyActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
			float newTime = SnapTime(xToTime(ImGui::GetMousePos().x));
			float minTime = (keyIndex > 0) ? curve.keys[keyIndex - 1].time + kTimeSnap : 0.0f;
			float maxTime = (keyIndex + 1 < static_cast<int>(curve.keys.size())) ? curve.keys[keyIndex + 1].time - kTimeSnap : 1.0e6f;
			key.time = std::clamp(newTime, minTime, maxTime);
		}

		if (ImGui::BeginPopupContextItem("CurveKeyContext")) {
			selectedKeyIndex_ = keyIndex;
			if (ImGui::MenuItem("Copy Key")) {
				copiedKey_ = key;
				hasCopiedKey_ = true;
			}
			ImGui::Separator();
			DrawKeyPresetMenuItems(curve, keyIndex);
			ImGui::Separator();
			if (ImGui::MenuItem("Delete Key")) {
				removeKeyIndex = keyIndex;
			}
			ImGui::EndPopup();
		}

		DrawKeyDiamond(drawList, keyCenter, KeyColor(keyIndex == selectedKeyIndex_ || keyActive));
		ImGui::PopID();
	}

	// --- 選択キーのタンジェントハンドル ---
	if (selectedKeyIndex_ >= 0 && selectedKeyIndex_ < static_cast<int>(curve.keys.size())) {
		AnimationKeyframe& key = curve.keys[selectedKeyIndex_];
		ImVec2 keyCenter(timeToX(key.time), valueToY(key.value));
		constexpr float kHandleLength = 55.0f;
		constexpr float kGripHalfSize = 6.0f;
		const ImU32 handleColor = IM_COL32(230, 230, 235, 255);

		// tangent(値/秒)をスクリーン方向ベクトルへ変換して固定長のハンドルにする。
		auto drawHandle = [&](float tangent, bool isOut, const char* id) {
			if (!std::isfinite(tangent)) {
				return; // Constant(ステップ)はハンドル無し。プリセットで解除する。
			}
			float directionSign = isOut ? 1.0f : -1.0f;
			ImVec2 direction(pixelsPerSecond * directionSign, -tangent * pixelsPerValue * directionSign);
			float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			if (length < 1.0e-3f) {
				return;
			}
			ImVec2 grip(keyCenter.x + direction.x / length * kHandleLength, keyCenter.y + direction.y / length * kHandleLength);

			drawList->AddLine(keyCenter, grip, handleColor, 1.0f);
			drawList->AddCircleFilled(grip, 4.0f, handleColor);

			ImGui::SetCursorScreenPos(ImVec2(grip.x - kGripHalfSize, grip.y - kGripHalfSize));
			ImGui::InvisibleButton(id, ImVec2(kGripHalfSize * 2.0f, kGripHalfSize * 2.0f));
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
				ImVec2 mouse = ImGui::GetMousePos();
				float deltaXPixels = mouse.x - keyCenter.x;
				// ハンドルの向き(in=左/out=右)を維持しつつ、ほぼ垂直ドラッグでも発散しないよう最小幅を確保。
				deltaXPixels = isOut ? (std::max)(deltaXPixels, 4.0f) : (std::min)(deltaXPixels, -4.0f);
				float deltaYPixels = mouse.y - keyCenter.y;
				float newTangent = (-deltaYPixels / pixelsPerValue) / (deltaXPixels / pixelsPerSecond);
				if (isOut) {
					key.outTangent = newTangent;
				} else {
					key.inTangent = newTangent;
				}
			}
		};

		// イージング指定された区間はタンジェント補間ではないためハンドルを出さない。
		// in側は前のキーのeasing、out側は自分のeasingが対象区間を決める。
		bool inSegmentIsTangent = (selectedKeyIndex_ == 0) || (curve.keys[selectedKeyIndex_ - 1].easing == EasingType::None);
		bool outSegmentIsTangent = (key.easing == EasingType::None);
		if (inSegmentIsTangent) {
			drawHandle(key.inTangent, false, "##InHandle");
		}
		if (outSegmentIsTangent) {
			drawHandle(key.outTangent, true, "##OutHandle");
		}
	}

	if (removeKeyIndex >= 0) {
		curve.keys.erase(curve.keys.begin() + removeKeyIndex);
		if (selectedKeyIndex_ >= static_cast<int>(curve.keys.size())) {
			selectedKeyIndex_ = static_cast<int>(curve.keys.size()) - 1;
		}
	}

	// --- プレイヘッド ---
	float playheadX = timeToX(previewTime_);
	drawList->AddLine(ImVec2(playheadX, canvasPosition.y), ImVec2(playheadX, canvasPosition.y + canvasSize.y), IM_COL32(255, 90, 90, 255), 2.0f);
	drawList->AddTriangleFilled(ImVec2(playheadX - 5.0f, canvasPosition.y), ImVec2(playheadX + 5.0f, canvasPosition.y), ImVec2(playheadX, canvasPosition.y + 8.0f), IM_COL32(255, 90, 90, 255));

	ImGui::SetCursorScreenPos(canvasPosition);
	ImGui::Dummy(canvasSize);
	AcceptClipDrop(animator);
}

void AnimationWindow::Draw(bool* pOpen) {
	// クリップを開けた場合のみ後段で有効化する(早期returnでは常に無効)。
	SetAnimationKeyContextEnabled(false);

	ImGui::Begin("Animation", pOpen);

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	// Play中はプレビューと編集を止める(ランタイム再生と競合するため)。
	if (IsGamePlaying()) {
		if (previewing_) {
			// Play開始時のシーン再構築で状態は上書きされるため、スナップショットは破棄する。
			previewing_ = false;
			previewPlaying_ = false;
			previewSnapshotJson_.clear();
			previewOwnerInstanceId_.clear();
		}
		recording_ = false;
		SetAnimationRecording(false);
		ImGui::TextDisabled("Playing... (editing disabled)");
		ImGui::End();
		return;
	}

	GameObject* owner = nullptr;
	AnimatorComponent* animator = FindSelectedAnimator(owner);

	// プレビュー対象が選択解除/変更されたら巻き戻す。
	if (previewing_) {
		bool ownerChanged = !owner || owner->GetInstanceId() != previewOwnerInstanceId_;
		if (ownerChanged || !animator) {
			EndPreview(*scene);
			// EndPreviewでシーンが再構築されるためポインタを取り直す。
			animator = FindSelectedAnimator(owner);
		}
	}

	if (!owner) {
		ImGui::TextDisabled("Select a GameObject.");
		ImGui::End();
		return;
	}

	if (!animator) {
		ImGui::TextDisabled("Selected object has no Animator.");
		if (ImGui::Button("Add AnimatorComponent")) {
			Component* added = owner->AddComponent(ComponentFactory::GetInstance().Create("AnimatorComponent"));
			scene->OnEditorComponentAdded(owner, added);
		}
		ImGui::End();
		return;
	}

	DrawClipBar(*animator);

	if (!animator->HasClip()) {
		ImGui::TextDisabled("No clip selected. [+]で作成するか、Projectから.anim.jsonをドロップしてください。");
		ImGui::End();
		return;
	}

	std::vector<NamedChannel> channels;
	CollectChannels(*owner, *animator, channels);

	// この時点でクリップ編集可能なので、Inspector側のAdd Keyframe/録画を有効化する。
	SetAnimationKeyContextEnabled(true);

	// Inspector(このフレームで先に描画済み)から報告された変更をキーとして取り込む。
	// 録画中の値変更と、フィールド右クリックのAdd Keyframeの両方がここへ届く。
	for (const AnimationRecordedChange& change : ConsumeAnimationRecordedChanges()) {
		if (!FindChannelValue(channels, change.path)) {
			continue; // バインドできないチャンネルはトラック化しない。
		}
		AddKeyAtTime(*animator, change.path, channels, previewTime_);
	}

	DrawToolbar(*scene, *animator, channels);
	ImGui::Separator();

	// 左: トラック一覧 / 右: ドープシート。
	ImGui::BeginChild("##TrackListPane", ImVec2(kTrackListWidth, 0.0f));
	// 先頭のAdd Trackボタンが右ペインのルーラーとほぼ同じ高さのヘッダーになる。
	DrawTrackList(*animator, channels);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("##DopeSheetPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
	if (viewMode_ == 0) {
		DrawDopeSheet(*animator, channels);
	} else {
		DrawCurveEditor(*animator, channels);
	}
	ImGui::EndChild();

	UpdatePreview(*animator);

	ImGui::End();

	// ウィンドウが[x]で閉じられたらプレビューを巻き戻す。
	if (pOpen && !*pOpen && previewing_) {
		EndPreview(*scene);
	}
}

void AnimationWindow::NotifyHidden() {
	SetAnimationKeyContextEnabled(false);
	if (recording_) {
		recording_ = false;
		SetAnimationRecording(false);
	}
	if (previewing_) {
		Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
		if (scene) {
			EndPreview(*scene);
		} else {
			previewing_ = false;
			previewPlaying_ = false;
			previewSnapshotJson_.clear();
			previewOwnerInstanceId_.clear();
		}
	}
}

#else // USE_IMGUI

void AnimationWindow::Draw(bool* pOpen) { (void)pOpen; }

void AnimationWindow::NotifyHidden() {}

#endif // USE_IMGUI

} // namespace KujakuEngine
