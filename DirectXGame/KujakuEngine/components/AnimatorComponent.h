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
/// アニメーション書き込み先チャンネル1件。pathは
/// 自身のComponent = "ComponentTypeName/チャンネル名"、
/// 子孫のComponent = "子/孫:ComponentTypeName/チャンネル名" 形式(Unityの相対パス相当)。
/// </summary>
struct AnimatorChannel {
	std::string path;
	float* value = nullptr;
	// bool型チャンネル(補間せずカーブ値>=0.5でtrue)。valueとどちらか一方を設定する。
	bool* boolValue = nullptr;
};

/// <summary>
/// AnimationClipを再生し、同じGameObjectのComponentのプロパティへ書き込むComponent(Unity Animator相当)。
/// 複数クリップを保持し、currentで選択中の1つを再生する。
/// トラックpathは "ComponentTypeName/チャンネル名" 形式で、CollectAnimatableChannelsで解決する。
/// </summary>
class KUJAKU_API AnimatorComponent : public Component {
public:
	/// <summary>
	/// 向き基準の移動チャンネル(Transform/moveForward・moveRight・moveUp)の適用先。
	/// axis: 0=右(+X), 1=上(+Y), 2=前(+Z)。
	/// </summary>
	struct LocalMoveTarget {
		GameObject* object = nullptr;
		int axis = 0;
	};

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

	/// <summary>
	/// 加算(additive)トラックの基準値をクリアします。次の評価時に「その時点の現在値」を基準として取り直します。
	/// 再生開始・プレビュー開始のタイミングで呼びます。
	/// </summary>
	void ClearAdditiveBases() {
		additiveBaseValues_.clear();
		localMoveLastValues_.clear();
		loopCycleCount_ = 0;
	}

	/// <summary>
	/// 加算トラックのキャプチャ済み基準値を取得します(未キャプチャならfalse)。
	/// Editorの録画で「現在値-基準値」の差分をキーにするために使います。
	/// </summary>
	bool TryGetAdditiveBase(const std::string& trackPath, float& outBase) const {
		auto found = additiveBaseValues_.find(trackPath);
		if (found == additiveBaseValues_.end()) {
			return false;
		}
		outBase = found->second;
		return true;
	}

	/// <summary>
	/// rootとその子孫全てのアニメーション可能チャンネルを列挙します(AnimationWindowと共用)。
	/// excludeComponentは列挙から除外します(Animator自身を渡す)。
	/// </summary>
	static void CollectHierarchyChannels(GameObject& root, const Component* excludeComponent, std::vector<AnimatorChannel>& outChannels);

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
	// bool型チャンネルpath→書き込み先(補間せずカーブ値>=0.5でtrue)。
	std::unordered_map<std::string, bool*> boolChannelMap_;
	size_t resolvedComponentCount_ = 0;
	bool bindingsDirty_ = true;

	// 加算トラックの基準値(path→再生開始時点の値)。初回評価時に遅延キャプチャする。
	std::unordered_map<std::string, float> additiveBaseValues_;
	// Loop時の周回数。周回ごとに加算トラックの基準値へ1周分の差分を積む(前進が累積する)。
	int loopCycleCount_ = 0;

	// 向き基準移動チャンネルの適用先(path→対象GameObject+軸)と、前回評価時のカーブ値(増分計算用)。
	std::unordered_map<std::string, LocalMoveTarget> localMoveTargets_;
	std::unordered_map<std::string, float> localMoveLastValues_;
};

} // namespace KujakuEngine
