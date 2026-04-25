#pragma once

#include <cstdint>
#include <string>

// 追加モジュール
#include "../externals/DirectXTex/DirectXTex.h"
#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"

// エンジン内部モジュール
#include "base/DirectXCommon.h"
#include "base/WinApp.h"
#include "base/Logger.h"
#include "base/StringUtil.h"

#include "2d/ImGuiManager.h"
#include "2d/Sprite.h"

#include "3d/GraphicsPipeline.h"
#include "3d/Model.h"
#include "3d/WorldTransform.h"
#include "3d/DIrectionalLight.h"
#include "3d/Camera.h"
#include "3d/DebugCamera.h"

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix3x3.h"
#include "math/Matrix4x4.h"
#include "math/Easing.h"
#include "math/Random.h"

#include "shapes/Rect.h"
#include "shapes/AABB.h"
#include "shapes/ShapeUtil.h"

#include "input/Input.h"

namespace KujakuEngine {

/// <summary>
/// エンジンの初期化
/// </summary>
/// <param name="title">ウィンドウタイトル</param>
/// <param name="enableDebugLayer">デバッグレイヤーを有効にするか</param>
void Initialize(const std::wstring& title = L"KujakuEngine", bool enableDebugLayer = false);

/// <summary>
/// エンジンの終了処理
/// </summary>
void Finalize();

/// <summary>
/// フレーム更新（メッセージ処理）
/// </summary>
/// <returns>falseで終了リクエスト</returns>
bool Update();

/// <summary>
/// 描画前処理
/// GraphicsPipeline・DirectXCommon のPreDrawをまとめて呼ぶ
/// </summary>
void PreDraw();

/// <summary>
/// 描画後処理
/// ImGui描画 + DirectXCommon のPostDrawをまとめて呼ぶ
/// </summary>
void PostDraw();

} // namespace KujakuEngine
