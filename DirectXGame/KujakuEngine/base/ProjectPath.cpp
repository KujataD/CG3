#include "ProjectPath.h"
#include <system_error>
#include <vector>
#include <Windows.h>

namespace KujakuEngine {

std::filesystem::path NormalizeEditorPath(const std::filesystem::path& path) {
	std::error_code error;
	std::filesystem::path normalized = std::filesystem::weakly_canonical(path, error);
	if (error) {
		return path.lexically_normal();
	}
	return normalized.lexically_normal();
}

std::filesystem::path DetectEditorProjectRoot() {
	std::error_code error;
	std::filesystem::path current = std::filesystem::current_path(error);
	if (error) {
		return std::filesystem::path(".");
	}

	std::filesystem::path cursor = current;
	while (!cursor.empty()) {
		if (std::filesystem::exists(cursor / "KujakuEngine.vcxproj")) {
			return NormalizeEditorPath(cursor);
		}

		std::filesystem::path directXGameProject = cursor / "DirectXGame" / "KujakuEngine.vcxproj";
		if (std::filesystem::exists(directXGameProject)) {
			return NormalizeEditorPath(cursor / "DirectXGame");
		}

		if (std::filesystem::exists(cursor / "KujakuEngine.sln")) {
			std::filesystem::path directXGameDirectory = cursor / "DirectXGame";
			if (std::filesystem::exists(directXGameDirectory)) {
				return NormalizeEditorPath(directXGameDirectory);
			}
			return NormalizeEditorPath(cursor);
		}

		std::filesystem::path parent = cursor.parent_path();
		if (parent == cursor) {
			break;
		}
		cursor = parent;
	}

	return NormalizeEditorPath(current);
}

std::filesystem::path GetExecutableDirectory() {
	std::vector<wchar_t> buffer(MAX_PATH);
	for (;;) {
		DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0) {
			return std::filesystem::path(".");
		}
		if (length < buffer.size()) {
			return std::filesystem::path(buffer.data()).parent_path();
		}
		// バッファ不足(ERROR_INSUFFICIENT_BUFFER)時は拡大して再試行する。
		buffer.resize(buffer.size() * 2);
	}
}

std::filesystem::path GetProjectDataRoot() {
	return DetectEditorProjectRoot() / "Data";
}

} // namespace KujakuEngine
