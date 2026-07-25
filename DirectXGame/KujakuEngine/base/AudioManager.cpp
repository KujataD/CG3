#include "AudioManager.h"
#include "Logger.h"
#include <cstring>
#include <fstream>

#pragma comment(lib, "xaudio2.lib")

namespace KujakuEngine {

namespace {

struct ChunkHeader {
	char id[4];
	uint32_t size;
};

bool ChunkIdEquals(const ChunkHeader& header, const char* id) { return std::memcmp(header.id, id, 4) == 0; }

} // namespace

AudioManager* AudioManager::GetInstance() {
	static AudioManager instance;
	return &instance;
}

void AudioManager::Initialize() {
	if (initialized_) {
		return;
	}

	HRESULT result = XAudio2Create(&xaudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(result)) {
		Logger::Log("[Audio] XAudio2Create failed. hr=" + std::to_string(result));
		xaudio2_.Reset();
		return;
	}

	result = xaudio2_->CreateMasteringVoice(&masteringVoice_);
	if (FAILED(result)) {
		// マスターボイス生成失敗は再生デバイスが無い環境でも起きる。エンジン全体は止めず、音だけ無効化する。
		Logger::Log("[Audio] CreateMasteringVoice failed (no audio device?). hr=" + std::to_string(result));
		masteringVoice_ = nullptr;
		xaudio2_.Reset();
		return;
	}

	initialized_ = true;
	Logger::Log("[Audio] AudioManager initialized.");
}

void AudioManager::Finalize() {
	for (auto& [handle, voice] : activeVoices_) {
		if (voice) {
			voice->Stop();
			voice->DestroyVoice();
		}
	}
	activeVoices_.clear();

	if (masteringVoice_) {
		masteringVoice_->DestroyVoice();
		masteringVoice_ = nullptr;
	}
	xaudio2_.Reset();
	initialized_ = false;
}

uint32_t AudioManager::LoadWav(const std::string& filePath) {
	auto found = soundHandleByPath_.find(filePath);
	if (found != soundHandleByPath_.end()) {
		return found->second;
	}

	std::ifstream file(filePath, std::ios::binary);
	if (!file) {
		Logger::Log("[Audio] Failed to open WAV: " + filePath);
		return kInvalidHandle;
	}

	// RIFFヘッダ("RIFF" + 全体サイズ + "WAVE")を確認する。
	ChunkHeader riff{};
	char waveId[4]{};
	file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
	file.read(waveId, sizeof(waveId));
	if (!file || !ChunkIdEquals(riff, "RIFF") || std::memcmp(waveId, "WAVE", 4) != 0) {
		Logger::Log("[Audio] Not a RIFF/WAVE file: " + filePath);
		return kInvalidHandle;
	}

	// fmt/data以外のチャンク(JUNK, LIST等)はスキップしながら両方を集める。
	SoundData sound{};
	ChunkHeader chunk{};
	while (file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk))) {
		if (ChunkIdEquals(chunk, "fmt ")) {
			sound.formatBytes.resize((std::max)(static_cast<size_t>(chunk.size), sizeof(WAVEFORMATEX)), 0);
			file.read(reinterpret_cast<char*>(sound.formatBytes.data()), chunk.size);
		} else if (ChunkIdEquals(chunk, "data")) {
			sound.pcmBytes.resize(chunk.size);
			file.read(reinterpret_cast<char*>(sound.pcmBytes.data()), chunk.size);
		} else {
			file.seekg(chunk.size, std::ios::cur);
		}
		// チャンクは2バイト境界に整列される(奇数サイズ時は1バイトのパディング)。
		if (chunk.size % 2 == 1) {
			file.seekg(1, std::ios::cur);
		}
	}

	if (sound.formatBytes.empty() || sound.pcmBytes.empty()) {
		Logger::Log("[Audio] fmt/data chunk not found: " + filePath);
		return kInvalidHandle;
	}

	uint32_t handle = static_cast<uint32_t>(sounds_.size());
	sounds_.push_back(std::move(sound));
	soundHandleByPath_[filePath] = handle;
	Logger::Log("[Audio] Loaded WAV: " + filePath);
	return handle;
}

uint32_t AudioManager::Play(uint32_t soundHandle, float volume, bool loop) {
	if (!initialized_ || soundHandle >= sounds_.size()) {
		return kInvalidHandle;
	}

	CollectFinishedVoices();

	const SoundData& sound = sounds_[soundHandle];
	const WAVEFORMATEX* format = reinterpret_cast<const WAVEFORMATEX*>(sound.formatBytes.data());

	IXAudio2SourceVoice* voice = nullptr;
	HRESULT result = xaudio2_->CreateSourceVoice(&voice, format);
	if (FAILED(result) || !voice) {
		Logger::Log("[Audio] CreateSourceVoice failed. hr=" + std::to_string(result));
		return kInvalidHandle;
	}

	XAUDIO2_BUFFER buffer{};
	buffer.pAudioData = sound.pcmBytes.data();
	buffer.AudioBytes = static_cast<UINT32>(sound.pcmBytes.size());
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

	if (FAILED(voice->SubmitSourceBuffer(&buffer))) {
		voice->DestroyVoice();
		return kInvalidHandle;
	}

	voice->SetVolume((std::max)(volume, 0.0f));
	voice->Start();

	uint32_t voiceHandle = nextVoiceHandle_++;
	activeVoices_[voiceHandle] = voice;
	return voiceHandle;
}

void AudioManager::Stop(uint32_t voiceHandle) {
	auto found = activeVoices_.find(voiceHandle);
	if (found == activeVoices_.end()) {
		return;
	}
	found->second->Stop();
	found->second->DestroyVoice();
	activeVoices_.erase(found);
}

void AudioManager::SetVolume(uint32_t voiceHandle, float volume) {
	auto found = activeVoices_.find(voiceHandle);
	if (found == activeVoices_.end()) {
		return;
	}
	found->second->SetVolume((std::max)(volume, 0.0f));
}

bool AudioManager::IsPlaying(uint32_t voiceHandle) {
	CollectFinishedVoices();
	return activeVoices_.find(voiceHandle) != activeVoices_.end();
}

void AudioManager::CollectFinishedVoices() {
	for (auto it = activeVoices_.begin(); it != activeVoices_.end();) {
		XAUDIO2_VOICE_STATE state{};
		it->second->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
		if (state.BuffersQueued == 0) {
			it->second->DestroyVoice();
			it = activeVoices_.erase(it);
		} else {
			++it;
		}
	}
}

} // namespace KujakuEngine
