#include "GameAudio.h"

#include "GameSettings.h"

#include <KujataEngine.h>
#include <base/AudioManager.h>

#include <array>
#include <string>
#include <unordered_map>

using namespace KujataEngine;

namespace {

/// <summary>効果音1種類ぶんの設定。</summary>
struct SeEntry {
	// Data相対のWAVパス。**今は全部mokugyoの仮音源。**本番音源はここだけ差し替える。
	const char* path;
	// 種類ごとの音量比(0〜1)。同じ音源しか無い今、鳴り分けの唯一の手がかりになっている。
	// 頻度の高い音ほど小さくしないと、戦闘中ずっと鳴り続けてうるさくなる。
	float volume;
};

constexpr const char* kPlaceholderSe = "Resources/mokugyo.wav";
constexpr const char* kFanfare = "Resources/fanfare.wav";

// Se enumと**必ず同じ並び**にすること(添字で引く)。
constexpr std::array<SeEntry, static_cast<size_t>(GameAudio::Se::Count)> kSeTable = {{
    {kPlaceholderSe, 0.25f}, // PlayerSwing  … 連打されるので控えめ
    {kPlaceholderSe, 0.55f}, // PlayerHit
    {kPlaceholderSe, 0.45f}, // Guard
    {kPlaceholderSe, 0.90f}, // JustGuard    … 成立を分からせたいので一番大きく
    {kPlaceholderSe, 0.70f}, // GuardBreak
    {kPlaceholderSe, 0.30f}, // Dodge
    {kPlaceholderSe, 0.65f}, // PlayerDamage
    {kPlaceholderSe, 0.85f}, // Critical
    {kPlaceholderSe, 0.35f}, // MagicShot
    {kPlaceholderSe, 0.60f}, // BossSlam
    {kPlaceholderSe, 0.50f}, // EnemyDown
    {kPlaceholderSe, 0.20f}, // UiMove       … カーソルを動かすたびなので最小
    {kPlaceholderSe, 0.40f}, // UiDecide
    {kPlaceholderSe, 0.30f}, // UiCancel
    {kPlaceholderSe, 0.80f}, // Death
    {kFanfare, 0.80f},       // Clear        … これだけ本物がある
}};

/// <summary>Data相対パスからサウンドハンドルを引く(読み込みは初回だけ)。</summary>
uint32_t AcquireSound(const std::string& relativePath) {
	static std::unordered_map<std::string, uint32_t> handleByPath;

	if (auto found = handleByPath.find(relativePath); found != handleByPath.end()) {
		return found->second;
	}

	const std::string absolutePath = (GetProjectDataRoot() / relativePath).string();
	const uint32_t handle = AudioManager::GetInstance()->LoadWav(absolutePath);
	// **失敗もキャッシュする。** 毎フレーム鳴らそうとするSEでファイルが無いと、
	// 毎回ディスクを叩きに行って盛大に重くなる。
	handleByPath[relativePath] = handle;
	if (handle == AudioManager::kInvalidHandle) {
		Logger::Log("[GameAudio] wav load failed: " + absolutePath);
	}
	return handle;
}

/// <summary>再生中のBGM。1曲しか鳴らさないので単一で持つ。</summary>
struct BgmState {
	std::string path;
	uint32_t voice = AudioManager::kInvalidHandle;
};

BgmState& Bgm() {
	static BgmState state;
	return state;
}

} // namespace

namespace GameAudio {

void PreloadSe() {
	// 同じパスはAcquireSound側のキャッシュで1回しか読まれないので、重複は気にしなくてよい。
	for (const SeEntry& entry : kSeTable) {
		AcquireSound(entry.path);
	}
}

void PlaySe(Se se) {
	const size_t index = static_cast<size_t>(se);
	if (index >= kSeTable.size()) {
		return;
	}
	const float volume = kSeTable[index].volume * GameSettings::SeVolumeRef();
	if (volume <= 0.0f) {
		return; // 消音時はボイスすら作らない。
	}

	const uint32_t sound = AcquireSound(kSeTable[index].path);
	if (sound == AudioManager::kInvalidHandle) {
		return;
	}
	// ボイスハンドルは捨てる。SEは鳴らしっぱなしで、AudioManagerが終わった順に片付ける。
	AudioManager::GetInstance()->Play(sound, volume, false);
}

void PlayBgm(const std::string& relativePath) {
	BgmState& bgm = Bgm();
	if (bgm.path == relativePath && bgm.voice != AudioManager::kInvalidHandle) {
		return; // 同じ曲が既に鳴っている。頭から鳴らし直さない。
	}

	StopBgm();

	const uint32_t sound = AcquireSound(relativePath);
	if (sound == AudioManager::kInvalidHandle) {
		return;
	}
	bgm.voice = AudioManager::GetInstance()->Play(sound, GameSettings::BgmVolumeRef(), true);
	bgm.path = relativePath;
}

void StopBgm() {
	BgmState& bgm = Bgm();
	if (bgm.voice != AudioManager::kInvalidHandle) {
		AudioManager::GetInstance()->Stop(bgm.voice);
	}
	bgm.voice = AudioManager::kInvalidHandle;
	bgm.path.clear();
}

void RefreshVolumes() {
	BgmState& bgm = Bgm();
	if (bgm.voice != AudioManager::kInvalidHandle) {
		AudioManager::GetInstance()->SetVolume(bgm.voice, GameSettings::BgmVolumeRef());
	}
}

void Shutdown() { StopBgm(); }

} // namespace GameAudio
