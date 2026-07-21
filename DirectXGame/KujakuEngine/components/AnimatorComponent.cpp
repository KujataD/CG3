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

// objectPrefixは自身なら空、子孫なら"子/孫:"。子孫を再帰的に辿ってチャンネルを集める。
void CollectChannelsRecursive(GameObject& object, const std::string& objectPrefix, const Component* excludeComponent, std::vector<AnimatorChannel>& outChannels) {
	std::vector<AnimatableChannel> channels;
	for (const std::unique_ptr<Component>& component : object.GetComponents()) {
		if (!component || component.get() == excludeComponent) {
			continue;
		}
		channels.clear();
		component->CollectAnimatableChannels(channels);
		std::string prefix = objectPrefix + component->GetTypeName() + "/";
		for (const AnimatableChannel& channel : channels) {
			outChannels.push_back({prefix + channel.path, channel.value, channel.boolValue});
		}
	}

	for (GameObject* child : object.GetChildren()) {
		if (!child) {
			continue;
		}
		std::string childPrefix;
		if (objectPrefix.empty()) {
			childPrefix = child->GetName() + ":";
		} else {
			// "親:" → "親/子:" と伸ばす。
			childPrefix = objectPrefix.substr(0, objectPrefix.size() - 1) + "/" + child->GetName() + ":";
		}
		CollectChannelsRecursive(*child, childPrefix, excludeComponent, outChannels);
	}
}

// 向き基準移動チャンネル(Transform/moveForward等)の適用先を再帰的に集める。
// pathの形式はCollectChannelsRecursiveと同一。
void CollectLocalMoveTargetsRecursive(GameObject& object, const std::string& objectPrefix, std::unordered_map<std::string, AnimatorComponent::LocalMoveTarget>& outTargets) {
	std::string transformPrefix = objectPrefix + "Transform/";
	outTargets.emplace(transformPrefix + "moveRight", AnimatorComponent::LocalMoveTarget{&object, 0});
	outTargets.emplace(transformPrefix + "moveUp", AnimatorComponent::LocalMoveTarget{&object, 1});
	outTargets.emplace(transformPrefix + "moveForward", AnimatorComponent::LocalMoveTarget{&object, 2});

	for (GameObject* child : object.GetChildren()) {
		if (!child) {
			continue;
		}
		std::string childPrefix;
		if (objectPrefix.empty()) {
			childPrefix = child->GetName() + ":";
		} else {
			childPrefix = objectPrefix.substr(0, objectPrefix.size() - 1) + "/" + child->GetName() + ":";
		}
		CollectLocalMoveTargetsRecursive(*child, childPrefix, outTargets);
	}
}

// 自身+子孫の総Component数(バインディング再解決の変化検知用)。
size_t CountHierarchyComponents(GameObject& object) {
	size_t count = object.GetComponents().size();
	for (GameObject* child : object.GetChildren()) {
		if (child) {
			count += CountHierarchyComponents(*child);
		}
	}
	return count;
}

} // namespace

void AnimatorComponent::CollectHierarchyChannels(GameObject& root, const Component* excludeComponent, std::vector<AnimatorChannel>& outChannels) {
	CollectChannelsRecursive(root, "", excludeComponent, outChannels);
}

void AnimatorComponent::Update() {
	if (!IsGamePlaying() || !playing_ || !clipLoaded_) {
		return;
	}

	float duration = clip_.GetDuration();
	if (duration <= 0.0f) {
		return;
	}

	time_ += Time::GetDeltaTime() * speed_;

	// Loopの周回をまたいだら、加算トラックの基準値へ1周分の差分を積む(前進などが周回ごとに累積する)。
	if (clip_.wrapMode == AnimationWrapMode::Loop) {
		int cycle = static_cast<int>(time_ / duration);
		if (cycle != loopCycleCount_) {
			int cycleDelta = cycle - loopCycleCount_;
			loopCycleCount_ = cycle;
			for (const AnimationTrack& track : clip_.tracks) {
				float cycleValueDelta = (track.curve.Evaluate(duration) - track.curve.Evaluate(0.0f)) * static_cast<float>(cycleDelta);

				if (track.additive) {
					auto base = additiveBaseValues_.find(track.path);
					if (base != additiveBaseValues_.end()) {
						base->second += cycleValueDelta;
					}
				}

				// 向き基準移動チャンネルは、周回時にlast値を1周分戻して増分が連続するようにする
				// (これが無いと周回の瞬間に開始位置へ引き戻される)。
				if (localMoveTargets_.count(track.path) != 0) {
					auto last = localMoveLastValues_.find(track.path);
					if (last != localMoveLastValues_.end()) {
						last->second -= cycleValueDelta;
					}
				}
			}
		}
	}

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
	ClearAdditiveBases();
	time_ = 0.0f;
	playing_ = playOnStart_ && clipLoaded_;
}

void AnimatorComponent::OnPlayStop() {
	playing_ = false;
	time_ = 0.0f;
	ClearAdditiveBases();
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
	ClearAdditiveBases();
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
		// 加算トラックは再生開始時点の値を基準にする(次の評価で遅延キャプチャ)。
		ClearAdditiveBases();
		playing_ = true;
	}
}

void AnimatorComponent::Stop() {
	playing_ = false;
}

void AnimatorComponent::EvaluateAt(float time) {
	// 自身+子孫のComponent構成が変わったらチャンネルの解決をやり直す(追加/削除/子の増減を検知)。
	GameObject* owner = GetOwner();
	if (owner && CountHierarchyComponents(*owner) != resolvedComponentCount_) {
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

		float value = track.curve.Evaluate(time);
		if (track.additive) {
			// 加算モード: 初回評価時の現在値を基準として遅延キャプチャし、カーブ値を差分として足す。
			auto base = additiveBaseValues_.find(track.path);
			if (base == additiveBaseValues_.end()) {
				base = additiveBaseValues_.emplace(track.path, *found->second).first;
			}
			value += base->second;
		}
		*found->second = value;
	}

	// --- bool型チャンネル ---
	// カーブ値を補間して閾値0.5で0/1に量子化する。エルミート補間で完全に1.0へ届かなくても、
	// 0.5を超えていればtrueになるため、攻撃判定などのON/OFFが確実に切り替わる。
	for (const AnimationTrack& track : clip_.tracks) {
		auto found = boolChannelMap_.find(track.path);
		if (found == boolChannelMap_.end() || !found->second) {
			continue;
		}
		*found->second = track.curve.Evaluate(time) >= 0.5f;
	}

	// --- 向き基準の移動チャンネル(moveForward/moveRight/moveUp) ---
	// カーブ値の「前回評価からの増分」を、その時点の自身の向きで回してtranslationへ加算する。
	// 移動中に旋回すると軌道が曲がる(ルートモーション風)。
	for (const AnimationTrack& track : clip_.tracks) {
		auto target = localMoveTargets_.find(track.path);
		if (target == localMoveTargets_.end() || !target->second.object) {
			continue;
		}

		float value = track.curve.Evaluate(time);
		auto last = localMoveLastValues_.find(track.path);
		if (last == localMoveLastValues_.end()) {
			// 初回は基準を取るだけ(評価開始時点の値からの増分にする)。
			localMoveLastValues_.emplace(track.path, value);
			continue;
		}
		float delta = value - last->second;
		last->second = value;
		if (std::abs(delta) < 1.0e-7f) {
			continue;
		}

		Vector3 localAxis = {0.0f, 0.0f, 0.0f};
		if (target->second.axis == 0) {
			localAxis.x = 1.0f;
		} else if (target->second.axis == 1) {
			localAxis.y = 1.0f;
		} else {
			localAxis.z = 1.0f;
		}

		WorldTransform& transform = target->second.object->GetTransform();
		transform.translation_ += transform.GetRotationQuaternion().RotateVector(localAxis * delta);
	}
}

void AnimatorComponent::ResolveBindings() {
	channelMap_.clear();
	boolChannelMap_.clear();
	bindingsDirty_ = false;
	resolvedComponentCount_ = 0;

	GameObject* owner = GetOwner();
	if (!owner) {
		return;
	}
	resolvedComponentCount_ = CountHierarchyComponents(*owner);

	// 自身と子孫全てのチャンネル("[子/孫:]ComponentTypeName/チャンネル名") → float*/bool* の対応表を作る。
	// 同種Component/同名の子が複数ある場合は最初の1つを対象にする(try_emplace)。
	std::vector<AnimatorChannel> channels;
	CollectHierarchyChannels(*owner, this, channels);
	for (const AnimatorChannel& channel : channels) {
		if (channel.boolValue) {
			boolChannelMap_.try_emplace(channel.path, channel.boolValue);
		} else if (channel.value) {
			channelMap_.try_emplace(channel.path, channel.value);
		}
	}

	// 向き基準移動チャンネルの適用先(GameObject+軸)も解決し直す。
	localMoveTargets_.clear();
	CollectLocalMoveTargetsRecursive(*owner, "", localMoveTargets_);
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
