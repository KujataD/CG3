#include "AnimatorComponent.h"

#include "../base/Time.h"
#include "../runtime/AssetResolver.h"
#include "../runtime/InspectorUI.h"
#include "../runtime/PlayState.h"
#include "../scene/GameObject.h"
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <unordered_map>

namespace KujakuEngine {

namespace {

std::string ReadString(const nlohmann::json& json, const char* key, const std::string& defaultValue) {
	if (!json.contains(key) || !json.at(key).is_string()) {
		return defaultValue;
	}
	return json.at(key).get<std::string>();
}

const std::string kEmptyString;

} // namespace

void AnimatorComponent::Update() {
	if (!IsGamePlaying() || !playing_ || !clipLoaded_) {
		return;
	}

	float duration = clip_.GetDuration();
	if (duration <= 0.0f) {
		return;
	}

	time_ += Time::GetDeltaTime() * speed_;

	bool finished = false;
	float evaluateTime = clip_.WrapTime(time_, finished);
	if (finished) {
		playing_ = false;
	}

	EvaluateAt(evaluateTime);
}

void AnimatorComponent::OnPlayStart() {
	if (!clipLoaded_) {
		LoadClip();
	}
	ResolveBindings();
	time_ = 0.0f;
	playing_ = playOnStart_ && clipLoaded_;
}

void AnimatorComponent::OnPlayStop() {
	playing_ = false;
	time_ = 0.0f;
}

void AnimatorComponent::SetClipPath(const std::string& clipPath) {
	if (clipPath.empty()) {
		return;
	}

	IAssetResolver& assetDatabase = GetAssetResolver();
	std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath("", clipPath);
	if (resolvedPath.empty()) {
		return;
	}

	std::string assetId = assetDatabase.GetOrCreateAssetId(resolvedPath);
	std::string storedPath = assetDatabase.MakeProjectRelativePath(resolvedPath);
	if (storedPath.empty()) {
		storedPath = clipPath;
	}

	// 既にリストにあれば選択のみ、無ければ追加して選択する。
	for (int index = 0; index < static_cast<int>(clipReferences_.size()); ++index) {
		if (clipReferences_[index].path == storedPath || (!assetId.empty() && clipReferences_[index].assetId == assetId)) {
			SetCurrentClipIndex(index);
			return;
		}
	}

	clipReferences_.push_back({assetId, storedPath});
	SetCurrentClipIndex(static_cast<int>(clipReferences_.size()) - 1);
}

const std::string& AnimatorComponent::GetClipPath() const {
	if (currentClipIndex_ < 0 || currentClipIndex_ >= static_cast<int>(clipReferences_.size())) {
		return kEmptyString;
	}
	return clipReferences_[currentClipIndex_].path;
}

bool AnimatorComponent::SetCurrentClipIndex(int index) {
	if (index < 0 || index >= static_cast<int>(clipReferences_.size())) {
		return false;
	}
	currentClipIndex_ = index;
	time_ = 0.0f;
	return LoadClip();
}

void AnimatorComponent::RemoveClipAt(int index) {
	if (index < 0 || index >= static_cast<int>(clipReferences_.size())) {
		return;
	}
	clipReferences_.erase(clipReferences_.begin() + index);

	if (clipReferences_.empty()) {
		currentClipIndex_ = -1;
		clip_ = AnimationClipData{};
		clipLoaded_ = false;
		playing_ = false;
		return;
	}

	if (currentClipIndex_ >= static_cast<int>(clipReferences_.size())) {
		currentClipIndex_ = static_cast<int>(clipReferences_.size()) - 1;
	}
	LoadClip();
}

bool AnimatorComponent::PlayByName(const std::string& clipName) {
	IAssetResolver& assetDatabase = GetAssetResolver();
	for (int index = 0; index < static_cast<int>(clipReferences_.size()); ++index) {
		std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath(clipReferences_[index].assetId, clipReferences_[index].path);
		if (resolvedPath.empty()) {
			continue;
		}
		std::string message;
		AnimationClipData loaded;
		if (!AnimationClipAsset::Load(resolvedPath, loaded, message)) {
			continue;
		}
		if (loaded.name == clipName) {
			SetCurrentClipIndex(index);
			ResolveBindings();
			Play();
			return true;
		}
	}
	return false;
}

bool AnimatorComponent::SaveClip(std::string& message) const {
	if (!clipLoaded_ || GetClipPath().empty()) {
		message = "No clip to save.";
		return false;
	}

	IAssetResolver& assetDatabase = GetAssetResolver();
	const AnimationClipReference& reference = clipReferences_[currentClipIndex_];
	std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath(reference.assetId, reference.path);
	if (resolvedPath.empty()) {
		message = "Failed to resolve clip path: " + reference.path;
		return false;
	}

	return AnimationClipAsset::Save(resolvedPath, clip_, message);
}

void AnimatorComponent::Play() {
	if (!clipLoaded_) {
		LoadClip();
		ResolveBindings();
	}
	if (clipLoaded_) {
		time_ = 0.0f;
		playing_ = true;
	}
}

void AnimatorComponent::Stop() {
	playing_ = false;
}

void AnimatorComponent::EvaluateAt(float time) {
	// Component構成が変わったらチャンネルの解決をやり直す(追加/削除どちらも検知)。
	GameObject* owner = GetOwner();
	if (owner && owner->GetComponents().size() != resolvedComponentCount_) {
		bindingsDirty_ = true;
	}
	if (bindingsDirty_) {
		ResolveBindings();
	}

	// カーブは毎回clip_のトラックから直接参照する(トラック増減でvectorが再確保されるため、
	// カーブへのポインタをキャッシュしてはならない)。
	for (const AnimationTrack& track : clip_.tracks) {
		auto found = channelMap_.find(track.path);
		if (found == channelMap_.end() || !found->second) {
			continue;
		}
		*found->second = track.curve.Evaluate(time);
	}
}

void AnimatorComponent::ResolveBindings() {
	channelMap_.clear();
	bindingsDirty_ = false;
	resolvedComponentCount_ = 0;

	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}
	resolvedComponentCount_ = owner->GetComponents().size();

	// "ComponentTypeName/チャンネル名" → float* の対応表を作る。
	// 同種Componentが複数ある場合は最初の1つを対象にする(AllowMultiple対応は将来)。
	std::vector<AnimatableChannel> channels;
	for (const std::unique_ptr<Component>& component : owner->GetComponents()) {
		if (!component || component.get() == this) {
			continue;
		}
		channels.clear();
		component->CollectAnimatableChannels(channels);
		std::string prefix = std::string(component->GetTypeName()) + "/";
		for (const AnimatableChannel& channel : channels) {
			channelMap_.try_emplace(prefix + channel.path, channel.value);
		}
	}
}

bool AnimatorComponent::LoadClip() {
	clipLoaded_ = false;
	clip_ = AnimationClipData{};
	bindingsDirty_ = true;

	if (currentClipIndex_ < 0 || currentClipIndex_ >= static_cast<int>(clipReferences_.size())) {
		return false;
	}

	const AnimationClipReference& reference = clipReferences_[currentClipIndex_];
	IAssetResolver& assetDatabase = GetAssetResolver();
	std::filesystem::path resolvedPath = assetDatabase.ResolveAssetPath(reference.assetId, reference.path);
	if (resolvedPath.empty()) {
		return false;
	}

	std::string message;
	AnimationClipData loaded;
	if (!AnimationClipAsset::Load(resolvedPath, loaded, message)) {
		return false;
	}

	clip_ = std::move(loaded);
	clipLoaded_ = true;
	return true;
}

void AnimatorComponent::DrawInspector() {
#ifdef USE_IMGUI
	// クリップ一覧(選択中に*印)。行クリックで切替。
	if (clipReferences_.empty()) {
		InspectorUI::TextDisabled("No clips. (Animation WindowかProjectからD&Dで追加)");
	} else {
		InspectorUI::TextUnformatted("Clips:");
		int removeIndex = -1;
		for (int index = 0; index < static_cast<int>(clipReferences_.size()); ++index) {
			std::string label = (index == currentClipIndex_ ? "* " : "  ") + clipReferences_[index].path;
			InspectorUI::TextUnformatted(label.c_str());
			InspectorUI::SameLine();
			std::string selectLabel = "Select##clip" + std::to_string(index);
			if (index != currentClipIndex_ && InspectorUI::Button(selectLabel.c_str())) {
				SetCurrentClipIndex(index);
			}
			if (index == currentClipIndex_) {
				InspectorUI::TextDisabled("(current)");
			}
			InspectorUI::SameLine();
			std::string removeLabel = "X##clip" + std::to_string(index);
			if (InspectorUI::Button(removeLabel.c_str())) {
				removeIndex = index;
			}
		}
		if (removeIndex >= 0) {
			RemoveClipAt(removeIndex);
		}
	}

	// パス直接入力でのクリップ追加(D&Dの代替手段)。
	std::array<char, 256> pathBuffer{};
	if (InspectorUI::InputText("Add Clip Path", pathBuffer.data(), pathBuffer.size())) {
		if (pathBuffer[0] != '\0') {
			SetClipPath(pathBuffer.data());
		}
	}

	if (clipLoaded_) {
		std::string info = "Clip: " + clip_.name + "  Tracks: " + std::to_string(clip_.tracks.size()) + "  Duration: " + std::to_string(clip_.GetDuration()) + "  Wrap: " + AnimationClipAsset::ToString(clip_.wrapMode);
		InspectorUI::TextDisabled(info.c_str());
	}

	InspectorUI::Checkbox("Play On Start", &playOnStart_);
	InspectorUI::DragFloat("Speed", &speed_, 0.01f);

	if (InspectorUI::Button("Reload Clip")) {
		LoadClip();
		ResolveBindings();
	}
#endif // USE_IMGUI
}

void AnimatorComponent::WriteJson(nlohmann::json& json) const {
	nlohmann::json clipsJson = nlohmann::json::array();
	for (const AnimationClipReference& reference : clipReferences_) {
		nlohmann::json clipJson;
		clipJson["assetId"] = reference.assetId;
		clipJson["path"] = reference.path;
		clipsJson.push_back(clipJson);
	}
	json["clips"] = clipsJson;
	json["currentClip"] = currentClipIndex_;
	json["playOnStart"] = playOnStart_;
	json["speed"] = speed_;
}

void AnimatorComponent::ReadJson(const nlohmann::json& json) {
	clipReferences_.clear();
	currentClipIndex_ = -1;

	if (json.contains("clips") && json.at("clips").is_array()) {
		for (const nlohmann::json& clipJson : json.at("clips")) {
			if (!clipJson.is_object()) {
				continue;
			}
			AnimationClipReference reference;
			reference.assetId = ReadString(clipJson, "assetId", "");
			reference.path = ReadString(clipJson, "path", "");
			if (!reference.assetId.empty() || !reference.path.empty()) {
				clipReferences_.push_back(std::move(reference));
			}
		}
		if (json.contains("currentClip") && json.at("currentClip").is_number_integer()) {
			currentClipIndex_ = json.at("currentClip").get<int>();
		}
	} else {
		// 旧形式(単一clipAssetId/clipPath)からの互換読み込み。
		AnimationClipReference reference;
		reference.assetId = ReadString(json, "clipAssetId", "");
		reference.path = ReadString(json, "clipPath", "");
		if (!reference.assetId.empty() || !reference.path.empty()) {
			clipReferences_.push_back(std::move(reference));
			currentClipIndex_ = 0;
		}
	}

	if (currentClipIndex_ < 0 || currentClipIndex_ >= static_cast<int>(clipReferences_.size())) {
		currentClipIndex_ = clipReferences_.empty() ? -1 : 0;
	}

	if (json.contains("playOnStart") && json.at("playOnStart").is_boolean()) {
		playOnStart_ = json.at("playOnStart").get<bool>();
	}
	if (json.contains("speed") && json.at("speed").is_number()) {
		speed_ = json.at("speed").get<float>();
	}

	LoadClip();
	bindingsDirty_ = true;
}

} // namespace KujakuEngine
