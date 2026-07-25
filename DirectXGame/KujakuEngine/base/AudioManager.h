#pragma once

#include "../runtime/KujakuApi.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include <xaudio2.h>

namespace KujakuEngine {

/// <summary>
/// XAudio2によるサウンド再生基盤(TextureManagerと同じシングルトン構成)。
/// 対応フォーマットは .wav(PCM / IEEE float / WAVE_FORMAT_EXTENSIBLE)のみ。
/// サウンド(読み込み済みデータ)とボイス(再生中インスタンス)を別ハンドルで管理する。
/// </summary>
class KUJAKU_API AudioManager {
public:
	static constexpr uint32_t kInvalidHandle = 0xFFFFFFFFu;

	static AudioManager* GetInstance();

	/// <summary>XAudio2エンジンとマスターボイスを生成する。失敗時は以後の全操作が安全に無効化される。</summary>
	void Initialize();

	/// <summary>全ボイスとXAudio2エンジンを破棄する。Scene(AudioSourceComponent)破棄後に呼ぶこと。</summary>
	void Finalize();

	/// <summary>
	/// WAVファイルを読み込みサウンドハンドルを返す(同一パスは使い回す)。失敗時はkInvalidHandle。
	/// </summary>
	uint32_t LoadWav(const std::string& filePath);

	/// <summary>
	/// サウンドを再生し、ボイスハンドルを返す(失敗時はkInvalidHandle)。
	/// 同じサウンドを複数同時再生できる(ボイスは再生ごとに生成)。
	/// </summary>
	uint32_t Play(uint32_t soundHandle, float volume, bool loop);

	/// <summary>再生を停止しボイスを破棄する。無効ハンドルは無視。</summary>
	void Stop(uint32_t voiceHandle);

	/// <summary>再生中ボイスの音量を変更する(0.0〜)。無効ハンドルは無視。</summary>
	void SetVolume(uint32_t voiceHandle, float volume);

	bool IsPlaying(uint32_t voiceHandle);

private:
	AudioManager() = default;
	~AudioManager() = default;
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;

	struct SoundData {
		// WAVEFORMATEX(可変長のcbSize拡張含む)の生バイト列。先頭をWAVEFORMATEX*として解釈する。
		std::vector<uint8_t> formatBytes;
		std::vector<uint8_t> pcmBytes;
	};

	/// <summary>再生し終わったボイスを破棄してマップから取り除く(Play/IsPlaying時に遅延実行)。</summary>
	void CollectFinishedVoices();

	Microsoft::WRL::ComPtr<IXAudio2> xaudio2_;
	IXAudio2MasteringVoice* masteringVoice_ = nullptr;
	bool initialized_ = false;

	std::vector<SoundData> sounds_;
	std::unordered_map<std::string, uint32_t> soundHandleByPath_;
	std::unordered_map<uint32_t, IXAudio2SourceVoice*> activeVoices_;
	uint32_t nextVoiceHandle_ = 0;
};

} // namespace KujakuEngine
