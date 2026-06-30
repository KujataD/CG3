#pragma once

#include "GameModule.h"
#include <Windows.h>
#include <cstdint>
#include <filesystem>
#include <string>

namespace KujakuEngine {

/// <summary>
/// Game DLLを一時コピーしてLoadLibrary / FreeLibraryする
/// </summary>
class GameModuleLoader {
public:
	GameModuleLoader() = default;
	~GameModuleLoader();

	GameModuleLoader(const GameModuleLoader&) = delete;
	GameModuleLoader& operator=(const GameModuleLoader&) = delete;

	/// <summary>
	/// DLLを一時コピーして読み込む
	/// </summary>
	GameModuleLoadResult Load(const std::filesystem::path& dllPath, const std::filesystem::path& copyDirectory);

	/// <summary>
	/// 読み込み済みDLLを解放する
	/// </summary>
	void Unload();

	/// <summary>
	/// DLL読み込み済みかどうか
	/// </summary>
	bool IsLoaded() const { return moduleHandle_ != nullptr; }

	/// <summary>
	/// 読み込んだDLLのAPIを取得
	/// </summary>
	const GameModuleApi& GetApi() const { return api_; }

	/// <summary>
	/// 最後にコピーして読み込んだDLLパス
	/// </summary>
	const std::filesystem::path& GetLoadedCopiedDllPath() const { return loadedCopiedDllPath_; }

private:
	template <class T>
	bool LoadExport(const char* name, T& outFunc, std::string& message) const;

	static std::string GetLastWin32ErrorMessage(const std::string& prefix);
	static std::wstring ToWideString(const std::filesystem::path& path);

private:
	HMODULE moduleHandle_ = nullptr;
	GameModuleApi api_{};
	std::filesystem::path loadedCopiedDllPath_;
	uint32_t loadGeneration_ = 0;
};

} // namespace KujakuEngine
