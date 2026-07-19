#pragma once

#include "../assets/AnimationClipAsset.h"
#include "../runtime/KujakuApi.h"
#include "../scene/Component.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace KujakuEngine {

/// <summary>
/// AnimatorComponentが参照するクリップアセット1件。
/// </summary>
struct AnimationClipReference {
	std::string assetId;
	std::string path;
};

/// <summary>
/// AnimationClipを再生し、同じGameObjectのComponentのプロパティへ書き込むComponent(Unity Animator相当)。
/// 複数クリップを保持し、currentで選択中の1つを再生する。
/// トラックpathは "ComponentTypeName/チャンネル名" 形式で、CollectAnimatableChannelsで解決する。
/// </summary>
class KUJAKU_API AnimatorComponent : public Component {
public:
	const char* GetTypeName() const override { return "AnimatorComponent"; }
	bool AllowMultiple() const override { return false; }

	void Update() override;
	void OnPlayStart() override;
	void OnPlayStop() override;

	void DrawInspector() override;
	void WriteJson(nlohmann::json& json) const override;
	void ReadJson(const nlohmann::json& json) override;

	/// <summary>
	/// クリップアセット(*.anim.json)への参照を設定し、読み込みます。
	/// 既にリストにあるパスなら選択のみ、無ければリストへ追加して選択します。
	/// </summary>
	void SetClipPath(const std::string& clipPath);

	/// <summary>選択中クリップのプロジェクト相対パス(未選択なら空)。</summary>
	const std::string& GetClipPath() const;

	const std::vector<AnimationClipReference>& GetClipReferences() const { return clipReferences_; }

	int GetCurrentClipIndex() const { return currentClipIndex_; }

	/// <summary>
	/// 指定indexのクリップを選択して読み込みます。
	/// </summary>
	bool SetCurrentClipIndex(int index);

	/// <summary>
	/// リストから指定indexのクリップ参照を外します(アセット自体は削除しない)。
	/// </summary>
	void RemoveClipAt(int index);

	/// <summary>
	/// クリップ名(クリップJSONのname)で選択して再生します。ゲームコードからの切替用。
	/// </summary>
	bool PlayByName(const std::string& clipName);

	/// <summary>
	/// 読み込み済みクリップ。編集(AnimationWindow)からも直接触る。
	/// </summary>
	AnimationClipData& GetClip() { return clip_; }
	const AnimationClipData& GetClip() const { return clip_; }

	bool HasClip() const { return clipLoaded_; }

	/// <summary>
	/// 現在のクリップをアセットへ保存します。
	/// </summary>
	bool SaveClip(std::string& message) const;

	void Play();
	void Stop();
	bool IsPlaying() const { return playing_; }

	float GetTime() const { return time_; }
	void SetTime(float time) { time_ = time; }

	/// <summary>
	/// 指定時刻で全トラックを評価してプロパティへ書き込みます(スクラブプレビューにも使用)。
	/// </summary>
	void EvaluateAt(float time);

	/// <summary>
	/// チャンネルpath→書き込み先float*を解決し直します。Component構成変更後に呼びます。
	/// </summary>
	void ResolveBindings();

	void MarkBindingsDirty() { bindingsDirty_ = true; }

private:
	bool LoadClip();

	std::vector<AnimationClipReference> clipReferences_;
	int currentClipIndex_ = -1;
	bool playOnStart_ = true;
	float speed_ = 1.0f;

	AnimationClipData clip_;
	bool clipLoaded_ = false;

	bool playing_ = false;
	float time_ = 0.0f;

	// チャンネルpath→書き込み先。トラックのカーブは評価のたびにclip_から直接参照する
	// (カーブへのポインタはトラック追加/削除でダングリングするため保持しない)。
	std::unordered_map<std::string, float*> channelMap_;
	size_t resolvedComponentCount_ = 0;
	bool bindingsDirty_ = true;
};

} // namespace KujakuEngine
