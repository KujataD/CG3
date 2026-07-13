#pragma once

#include "KujakuApi.h"

namespace KujakuEngine {

// ゲームがPlay(実行)中かどうかを表す、ランタイム側が所有する状態。
// EditorがStart/Stopで設定し、ランタイム(Scene/Component/ゲームロジック)が参照する。
// Editorへの逆依存を作らないよう、状態そのものはランタイム層が持つ。
KUJAKU_API bool IsGamePlaying();

// Play/Edit状態を設定する(Editor側が呼ぶ)。
KUJAKU_API void SetGamePlaying(bool playing);

// Sceneビュー(ウィンドウ)がフォーカスされているか。Editorが毎フレーム設定する。
// これがtrueの時だけデバッグカメラをキーボード/マウスで操作する(プレイ中も含む)。
KUJAKU_API bool IsSceneViewFocused();

// Sceneビューのフォーカス状態を設定する(Editor側が呼ぶ)。
KUJAKU_API void SetSceneViewFocused(bool focused);

} // namespace KujakuEngine
