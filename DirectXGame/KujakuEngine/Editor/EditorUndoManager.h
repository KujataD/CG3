#pragma once

#include "../runtime/KujakuApi.h"
#include <cstddef>
#include <string>
#include <vector>

namespace KujakuEngine {

class Scene;

/// <summary>
/// Editor上のUndo/Redo履歴をSceneスナップショットで管理します。
/// </summary>
class EditorUndoManager {
public:
	/// <summary>
	/// Undo履歴1件分のScene状態。
	/// </summary>
	struct Snapshot {
		std::string label;
		std::string sceneJson;
		std::string selectedObjectInstanceId;
	};

	/// <summary>
	/// シングルトンインスタンスを取得します。
	/// </summary>
	static KUJAKU_API EditorUndoManager* GetInstance();

	/// <summary>
	/// 現在SceneをUndo履歴へ保存します。
	/// </summary>
	KUJAKU_API void Capture(Scene& scene, const std::string& label);

	/// <summary>
	/// 指定JSONをUndo履歴へ保存します。
	/// </summary>
	KUJAKU_API void Capture(Scene& scene, const std::string& label, const std::string& sceneJson);

	/// <summary>
	/// Undoを実行します。
	/// </summary>
	KUJAKU_API bool Undo(Scene& scene);

	/// <summary>
	/// Redoを実行します。
	/// </summary>
	KUJAKU_API bool Redo(Scene& scene);

	/// <summary>
	/// Undo/Redo履歴を破棄します。
	/// </summary>
	KUJAKU_API void Clear();

	/// <summary>
	/// Undo履歴上限を設定します。
	/// </summary>
	KUJAKU_API void SetMaxUndoCount(size_t maxUndoCount);

	/// <summary>
	/// Undo履歴上限を取得します。
	/// </summary>
	size_t GetMaxUndoCount() const { return maxUndoCount_; }

	/// <summary>
	/// Undo可能かを返します。
	/// </summary>
	bool CanUndo() const { return !undoStack_.empty(); }

	/// <summary>
	/// Redo可能かを返します。
	/// </summary>
	bool CanRedo() const { return !redoStack_.empty(); }

private:
	EditorUndoManager() = default;
	~EditorUndoManager() = default;
	EditorUndoManager(const EditorUndoManager&) = delete;
	EditorUndoManager& operator=(const EditorUndoManager&) = delete;

	Snapshot MakeSnapshot(Scene& scene, const std::string& label, const std::string& sceneJson) const;
	void PushUndo(Snapshot snapshot);
	void PushRedo(Snapshot snapshot);
	bool ApplySnapshot(Scene& scene, const Snapshot& snapshot);
	void TrimUndoStack();
	void RestoreSelection(Scene& scene, const std::string& selectedObjectInstanceId);

	size_t maxUndoCount_ = 20;
	std::vector<Snapshot> undoStack_;
	std::vector<Snapshot> redoStack_;
};

} // namespace KujakuEngine
