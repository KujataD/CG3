#include "ProjectPath.h"
#include <system_error>

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

} // namespace KujakuEngine
