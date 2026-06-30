#include "ImGuiManager.h"
#include "../../externals/imgui/imgui_internal.h"
#include "../../externals/ImGuizmo-1.9/src/ImGuizmo.h"
#include "../Editor/EditorApplication.h"
#include "../Editor/EditorSelection.h"
#include "../Editor/SceneJsonExporter.h"
#include "../3d/Camera.h"
#include "../3d/Model.h"
#include "../components/ModelRendererComponent.h"
#include "../components/TransformComponent.h"
#include "../base/DirectXCommon.h"
#include "../base/TextureManager.h"
#include "../base/WinApp.h"
#include "../scene/ComponentFactory.h"
#include "../math/MathUtil.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>

namespace KujakuEngine {
namespace {

constexpr float kDegreesToRadians = 0.017453292519943295f;
constexpr float kRayEpsilon = 0.000001f;

struct PickRay {
	Vector3 origin;
	Vector3 direction;
};

void AllocateImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	// ImGui 1.92系のDX12バックエンドは、フォントや追加テクスチャ用のSRVをバックエンド側へ要求してくる。
	// KujakuEngine側のSRV採番関数を使うことで、Gameウィンドウ用SRVや通常テクスチャと番号が衝突しないようにする。
	DirectXCommon* dxCommon = static_cast<DirectXCommon*>(info->UserData);
	uint32_t srvIndex = dxCommon->AllocateSrvIndex();
	ID3D12DescriptorHeap* srvHeap = dxCommon->GetSrvDescriptorHeap();
	uint32_t descriptorSize = dxCommon->GetDescriptorSizeSRV();

	// CPUハンドルはCreateShaderResourceViewなど、CPU側からDescriptorを書き込む用途で使う。
	*outCpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	outCpuHandle->ptr += descriptorSize * srvIndex;

	// GPUハンドルはシェーダーやImGui描画時にGPU側から参照する用途で使う。
	*outGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
	outGpuHandle->ptr += descriptorSize * srvIndex;
}

void FreeImGuiSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {
	// 現状はSRVヒープをフレーム中に再利用しないため解放処理は不要。
}

float* MatrixData(Matrix4x4& matrix) {
	return &matrix.m[0][0];
}

const float* MatrixData(const Matrix4x4& matrix) {
	return &matrix.m[0][0];
}

void ApplyCinderImGuiDarkStyle() {
	ImGuiStyle& style = ImGui::GetStyle();		// 現在のImGuiスタイル設定を参照で取得する
	style = ImGuiStyle();						// スタイル設定をデフォルト状態にリセットする

	style.WindowMinSize = ImVec2(160.0f, 20.0f);// ウィンドウの最小サイズ 
	style.FramePadding = ImVec2(4.0f, 2.0f);	// ボタンや入力欄などの内側余白 
	style.ItemSpacing = ImVec2(6.0f, 2.0f);		// UI要素同士の間隔 
	style.ItemInnerSpacing = ImVec2(2.0f, 4.0f);// 複合UI要素内のパーツ同士の間隔 
	style.Alpha = 0.95f;						// UI全体の透明度 
	style.WindowRounding = 4.0f;				// ウィンドウ角の丸み 
	style.FrameRounding = 2.0f;					// ボタンや入力欄などの角の丸み 
	style.ChildRounding = 5.0f;					// 子ウィンドウ角の丸み 
	style.PopupRounding = 5.0f;					// ポップアップウィンドウ角の丸み 
	style.IndentSpacing = 6.0f;					// ツリーなどでインデントする幅 
	style.ColumnsMinSpacing = 50.0f;			// カラム同士の最小間隔 
	style.GrabMinSize = 14.0f;					// スライダーやスクロールバーのつまみの最小サイズ 
	style.GrabRounding = 16.0f;					// スライダーやスクロールバーのつまみの角の丸み 
	style.ScrollbarSize = 12.0f;				// スクロールバーの太さ 
	style.ScrollbarRounding = 16.0f;			// スクロールバー角の丸み 
	style.TabRounding = 5.0f;					// タブ角の丸み 
	ImVec4* colors = style.Colors;

	const ImVec4 textColor = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
	const ImVec4 textDisabledColor = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
	const ImVec4 mainBgColor = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
	const ImVec4 mainBgTransparentColor = ImVec4(0.02f, 0.02f, 0.02f, 0.73f);
	const ImVec4 popupBgColor = ImVec4(0.02f, 0.02f, 0.02f, 0.97f);
	const ImVec4 titleCollapsedColor = ImVec4(0.02f, 0.02f, 0.02f, 0.75f);
	const ImVec4 menuBarBgColor = ImVec4(0.02f, 0.02f, 0.02f, 0.70f);

	const ImVec4 frameBgColor = ImVec4(0.110f, 0.110f, 0.110f, 1.00f);
	const ImVec4 tabBgColor = ImVec4(0.110f, 0.110f, 0.110f, 0.92f);
	const ImVec4 tableRowAltColor = ImVec4(0.110f, 0.110f, 0.110f, 0.25f);

	const ImVec4 accentColor = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
	const ImVec4 accentHoveredColor = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
	const ImVec4 accentHoveredStrong = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
	const ImVec4 accentHeaderColor = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
	const ImVec4 accentPreviewColor = ImVec4(0.92f, 0.18f, 0.29f, 0.50f);
	const ImVec4 accentSelectedBgColor = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
	const ImVec4 accentDropTargetColor = ImVec4(0.92f, 0.18f, 0.29f, 0.90f);

	const ImVec4 cyanColor = ImVec4(0.47f, 0.77f, 0.83f, 0.78f);
	const ImVec4 cyanWeakColor = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
	const ImVec4 cyanVeryWeakColor = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
	const ImVec4 cyanLineColor = ImVec4(0.47f, 0.77f, 0.83f, 0.80f);
	const ImVec4 cyanLineDimmedColor = ImVec4(0.47f, 0.77f, 0.83f, 0.40f);

	const ImVec4 borderColor = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
	const ImVec4 borderShadowColor = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	const ImVec4 scrollbarGrabColor = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
	const ImVec4 checkMarkColor = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
	const ImVec4 separatorColor = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
	const ImVec4 separatorLightColor = ImVec4(0.14f, 0.16f, 0.19f, 0.75f);
	const ImVec4 transparentRowColor = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
	const ImVec4 textHighlightColor = ImVec4(0.86f, 0.93f, 0.89f, 0.70f);
	const ImVec4 plotColor = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);

	colors[ImGuiCol_Text] = textColor;										 // 通常テキストの色 
	colors[ImGuiCol_TextDisabled] = textDisabledColor;                       // 無効状態のテキスト色 
	colors[ImGuiCol_WindowBg] = mainBgColor;                                 // 通常ウィンドウ背景色 
	colors[ImGuiCol_ChildBg] = mainBgColor;                                  // 子ウィンドウ背景色 
	colors[ImGuiCol_PopupBg] = popupBgColor;                                 // ポップアップ背景色 
	colors[ImGuiCol_Border] = borderColor;                                   // 枠線の色 
	colors[ImGuiCol_BorderShadow] = borderShadowColor;                       // 枠線の影色 
	colors[ImGuiCol_FrameBg] = frameBgColor;                                 // 入力欄やチェックボックスなどの背景色 
	colors[ImGuiCol_FrameBgHovered] = accentHoveredColor;                    // 入力欄などにマウスを重ねた時の背景色 
	colors[ImGuiCol_FrameBgActive] = accentColor;                            // 入力欄などを操作中の背景色 
	colors[ImGuiCol_TitleBg] = mainBgColor;                                  // 非アクティブなタイトルバー背景色 
	colors[ImGuiCol_TitleBgActive] = accentColor;                            // アクティブなタイトルバー背景色 
	colors[ImGuiCol_TitleBgCollapsed] = titleCollapsedColor;                 // 折りたたみ状態のタイトルバー背景色 
	colors[ImGuiCol_MenuBarBg] = menuBarBgColor;                             // メニューバー背景色 
	colors[ImGuiCol_ScrollbarBg] = mainBgColor;                              // スクロールバー背景色 
	colors[ImGuiCol_ScrollbarGrab] = scrollbarGrabColor;                     // スクロールバーのつまみ色 
	colors[ImGuiCol_ScrollbarGrabHovered] = accentHoveredColor;              // スクロールバーつまみにマウスを重ねた時の色 
	colors[ImGuiCol_ScrollbarGrabActive] = accentColor;                      // スクロールバーつまみを操作中の色 
	colors[ImGuiCol_CheckMark] = checkMarkColor;                             // チェックマークの色 
	colors[ImGuiCol_SliderGrab] = cyanWeakColor;                             // スライダーつまみの色 
	colors[ImGuiCol_SliderGrabActive] = accentColor;                         // スライダーつまみを操作中の色 
	colors[ImGuiCol_Button] = frameBgColor;                                  // 通常ボタンの色 
	colors[ImGuiCol_ButtonHovered] = accentHoveredStrong;                    // ボタンにマウスを重ねた時の色 
	colors[ImGuiCol_ButtonActive] = accentColor;                             // ボタンを押している時の色 
	colors[ImGuiCol_Header] = accentHeaderColor;                             // ヘッダーや選択項目の通常色 
	colors[ImGuiCol_HeaderHovered] = accentHoveredStrong;                    // ヘッダーや選択項目にマウスを重ねた時の色 
	colors[ImGuiCol_HeaderActive] = accentColor;                             // ヘッダーや選択項目を操作中の色 
	colors[ImGuiCol_Separator] = separatorColor;                             // 区切り線の通常色 
	colors[ImGuiCol_SeparatorHovered] = accentHoveredColor;                  // 区切り線にマウスを重ねた時の色 
	colors[ImGuiCol_SeparatorActive] = accentColor;                          // 区切り線を操作中の色 
	colors[ImGuiCol_ResizeGrip] = cyanVeryWeakColor;                         // ウィンドウリサイズつまみの通常色 
	colors[ImGuiCol_ResizeGripHovered] = accentHoveredColor;                 // リサイズつまみにマウスを重ねた時の色 
	colors[ImGuiCol_ResizeGripActive] = accentColor;                         // リサイズつまみを操作中の色 
	colors[ImGuiCol_Tab] = tabBgColor;                                       // 非選択タブの色 
	colors[ImGuiCol_TabHovered] = accentHoveredStrong;                       // タブにマウスを重ねた時の色 
	colors[ImGuiCol_TabSelected] = accentColor;                              // 選択中タブの色 
	colors[ImGuiCol_TabSelectedOverline] = cyanLineColor;                    // 選択中タブ上部ラインの色 
	colors[ImGuiCol_TabDimmed] = popupBgColor;                               // 暗く表示された非選択タブの色 
	colors[ImGuiCol_TabDimmedSelected] = frameBgColor;                       // 暗く表示された選択中タブの色 
	colors[ImGuiCol_TabDimmedSelectedOverline] = cyanLineDimmedColor;        // 暗く表示された選択中タブ上部ラインの色 
	colors[ImGuiCol_DockingPreview] = accentPreviewColor;                    // ドッキング先プレビューの色 
	colors[ImGuiCol_DockingEmptyBg] = mainBgColor;                           // ドッキング領域の空背景色 
	colors[ImGuiCol_PlotLines] = plotColor;                                  // 折れ線グラフの線色 
	colors[ImGuiCol_PlotLinesHovered] = accentColor;                         // 折れ線グラフにマウスを重ねた時の色 
	colors[ImGuiCol_PlotHistogram] = plotColor;                              // ヒストグラムの色 
	colors[ImGuiCol_PlotHistogramHovered] = accentColor;                     // ヒストグラムにマウスを重ねた時の色 
	colors[ImGuiCol_TableHeaderBg] = frameBgColor;                           // テーブルヘッダー背景色 
	colors[ImGuiCol_TableBorderStrong] = separatorColor;                     // テーブルの強い境界線色 
	colors[ImGuiCol_TableBorderLight] = separatorLightColor;                 // テーブルの弱い境界線色 
	colors[ImGuiCol_TableRowBg] = transparentRowColor;                       // テーブル行の通常背景色 
	colors[ImGuiCol_TableRowBgAlt] = tableRowAltColor;                       // テーブル交互行の背景色 
	colors[ImGuiCol_TextSelectedBg] = accentSelectedBgColor;                 // テキスト選択時の背景色 
	colors[ImGuiCol_DragDropTarget] = accentDropTargetColor;                 // ドラッグ＆ドロップ先の強調色 
	colors[ImGuiCol_NavCursor] = cyanColor;                                  // キーボード・ゲームパッド操作時のカーソル色 
	colors[ImGuiCol_NavWindowingHighlight] = textHighlightColor;             // ナビゲーション時のウィンドウ強調色 
	colors[ImGuiCol_NavWindowingDimBg] = mainBgTransparentColor;             // ナビゲーション時の背景暗転色 
	colors[ImGuiCol_ModalWindowDimBg] = mainBgTransparentColor;              // モーダルウィンドウ表示時の背景暗転色 
}

bool GetModelLocalBounds(const Model& model, Vector3& outMin, Vector3& outMax) {
	const std::vector<VertexData>& vertices = model.GetVertices();
	if (vertices.empty()) {
		return false;
	}

	outMin = {vertices[0].position.x, vertices[0].position.y, vertices[0].position.z};
	outMax = outMin;

	for (const VertexData& vertex : vertices) {
		outMin.x = (std::min)(outMin.x, vertex.position.x);
		outMin.y = (std::min)(outMin.y, vertex.position.y);
		outMin.z = (std::min)(outMin.z, vertex.position.z);
		outMax.x = (std::max)(outMax.x, vertex.position.x);
		outMax.y = (std::max)(outMax.y, vertex.position.y);
		outMax.z = (std::max)(outMax.z, vertex.position.z);
	}

	// 平面など厚みがないModelもクリックしやすいよう、薄い軸だけ少し広げる。
	if (std::fabs(outMax.x - outMin.x) < 0.001f) {
		outMin.x -= 0.05f;
		outMax.x += 0.05f;
	}
	if (std::fabs(outMax.y - outMin.y) < 0.001f) {
		outMin.y -= 0.05f;
		outMax.y += 0.05f;
	}
	if (std::fabs(outMax.z - outMin.z) < 0.001f) {
		outMin.z -= 0.05f;
		outMax.z += 0.05f;
	}

	return true;
}

bool UpdateRayAabbSlab(float origin, float direction, float minValue, float maxValue, float& inOutNear, float& inOutFar) {
	if (std::fabs(direction) < kRayEpsilon) {
		if (origin < minValue) {
			return false;
		}
		if (origin > maxValue) {
			return false;
		}
		return true;
	}

	float nearValue = (minValue - origin) / direction;
	float farValue = (maxValue - origin) / direction;
	if (nearValue > farValue) {
		std::swap(nearValue, farValue);
	}

	inOutNear = (std::max)(inOutNear, nearValue);
	inOutFar = (std::min)(inOutFar, farValue);
	if (inOutNear > inOutFar) {
		return false;
	}

	return true;
}

bool IntersectRayAabb(const PickRay& ray, const Vector3& minValue, const Vector3& maxValue, float& outDistance) {
	float nearDistance = 0.0f;
	float farDistance = (std::numeric_limits<float>::max)();

	if (!UpdateRayAabbSlab(ray.origin.x, ray.direction.x, minValue.x, maxValue.x, nearDistance, farDistance)) {
		return false;
	}
	if (!UpdateRayAabbSlab(ray.origin.y, ray.direction.y, minValue.y, maxValue.y, nearDistance, farDistance)) {
		return false;
	}
	if (!UpdateRayAabbSlab(ray.origin.z, ray.direction.z, minValue.z, maxValue.z, nearDistance, farDistance)) {
		return false;
	}

	outDistance = nearDistance;
	if (outDistance < 0.0f) {
		outDistance = farDistance;
	}
	if (outDistance < 0.0f) {
		return false;
	}

	return true;
}

bool HasPickableRenderer(GameObject& gameObject, ModelRendererComponent*& outRenderer) {
	for (const std::unique_ptr<Component>& component : gameObject.GetComponents()) {
		if (!component || !component->IsEnabled()) {
			continue;
		}

		ModelRendererComponent* renderer = dynamic_cast<ModelRendererComponent*>(component.get());
		if (!renderer) {
			continue;
		}
		if (!renderer->GetModel()) {
			continue;
		}

		outRenderer = renderer;
		return true;
	}

	return false;
}

} // namespace

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize() {
#ifdef USE_IMGUI

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	WinApp* winApp = WinApp::GetInstance();

	// ImGui初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Dockingを有効化する。これによりDockSpaceや各ウィンドウのドッキングが使える。
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// キーボード操作を有効化して、エディタUIとして最低限扱いやすくする。
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

	ApplyCinderImGuiDarkStyle();
	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// 1.92系のDX12バックエンドはInitInfoで初期化する。
	// 旧式の初期化ではフォントアトラス用テクスチャが正しく作られず、assertの原因になる。
	ImGui_ImplDX12_InitInfo initInfo{};
	initInfo.Device = dxCommon->GetDevice();
	initInfo.CommandQueue = dxCommon->GetCommandQueue();
	initInfo.NumFramesInFlight = dxCommon->GetSwapChainBufferCount();
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();
	initInfo.UserData = dxCommon;
	initInfo.SrvDescriptorAllocFn = AllocateImGuiSrvDescriptor;
	initInfo.SrvDescriptorFreeFn = FreeImGuiSrvDescriptor;
	ImGui_ImplDX12_Init(&initInfo);

	// 初期レイアウトはImGui初期化後、最初のDrawDockSpaceで構築する。
	dockLayoutInitialized_ = false;
	consoleLogs_.clear();
	AddConsoleLog("Console initialized.");
#endif // USE_IMGUI
}

void ImGuiManager::AddConsoleLog(const std::string& message) {
	consoleLogs_.push_back(message);
	// 無制限にログを溜めるとメモリと描画負荷が増えるため、簡易Consoleでは古いログから捨てる。
	if (consoleLogs_.size() > 128) {
		consoleLogs_.erase(consoleLogs_.begin());
	}
}

void ImGuiManager::ClearConsoleLogs() {
	consoleLogs_.clear();
}

void ImGuiManager::DrawDockSpace() {
#ifdef USE_IMGUI
	// MainViewport全体を覆う透明な親ウィンドウを作り、その中にDockSpaceを置く。
	// これによりGame/Hierarchy/Inspector/ConsoleをUnity風に分割配置できる。
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	// DockSpace用の親ウィンドウは移動・リサイズ・タイトルバー表示をさせず、画面全体の土台として使う。
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// 親ウィンドウの余白や角丸を消して、アプリ全体がエディタ領域になるようにする。
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("KujakuEditorDockSpace", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	if (ImGui::BeginMenuBar()) {
		// EditはStopと同じ扱い。今はPlayを止めて編集状態に戻すだけにしている。
		if (ImGui::Button("Edit")) {
			EditorApplication::GetInstance()->Stop();
		}
		ImGui::SameLine();
		// Startを押すとPlayへ入り、EditorApplication側でゲームロジックが進む。
		if (ImGui::Button("Start")) {
			EditorApplication::GetInstance()->Start();
		}
		ImGui::SameLine();
		// Stopを押すとEditへ戻り、EditorApplication側でゲームロジックが止まる。
		if (ImGui::Button("Stop")) {
			EditorApplication::GetInstance()->Stop();
		}
		ImGui::SameLine();

		// 現在のモードをメニューバー上にも出して、Start/Stopの結果をすぐ確認できるようにする。
		if (EditorApplication::GetInstance()->IsPlaying()) {
			ImGui::TextUnformatted("Mode: Play");
		} else {
			ImGui::TextUnformatted("Mode: Edit");
		}
		ImGui::EndMenuBar();
	}

	// DockSpaceのIDは毎フレーム同じ値にする必要がある。IDが変わるとドッキング状態を維持できない。
	ImGuiID dockspaceId = ImGui::GetID("KujakuEditorMainDockSpace");
	ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

	// DockBuilderによる初期配置は初回だけ。以降はユーザーがドラッグした配置を尊重する。
	if (!dockLayoutInitialized_) {
		SetupInitialLayout(dockspaceId);
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::SetupInitialLayout(ImGuiID dockspaceId) {
#ifdef USE_IMGUI
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// 既存ノードを一度消してから、Unityの2 by 3 Layoutに近い初期Dock構成を作る。
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
	ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

	// gameNodeを少しずつ分割して、Game / Hierarchy / Project / Inspector / Consoleを作る。
	ImGuiID hierarchyNode = 0;
	ImGuiID projectNode = 0;
	ImGuiID inspectorNode = 0;
	ImGuiID consoleNode = 0;
	ImGuiID gameNode = dockspaceId;

	// 右側にInspector、Project、Hierarchyの順で列を作る。
	// 画面上ではGame | Hierarchy | Project | Inspectorとなり、ProjectがHierarchyとInspectorの間に入る。
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.25f, &inspectorNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.20f, &projectNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Right, 0.20f, &hierarchyNode, &gameNode);
	ImGui::DockBuilderSplitNode(gameNode, ImGuiDir_Down, 0.28f, &consoleNode, &gameNode);

	// ウィンドウ名とDock先を紐づける。名前は各ImGui::Beginの文字列と一致させる。
	ImGui::DockBuilderDockWindow("Inspector", inspectorNode);
	ImGui::DockBuilderDockWindow("Project", projectNode);
	ImGui::DockBuilderDockWindow("Hierarchy", hierarchyNode);
	ImGui::DockBuilderDockWindow("Console", consoleNode);
	ImGui::DockBuilderDockWindow("Game", gameNode);
	ImGui::DockBuilderFinish(dockspaceId);

	// ここをtrueにして、次フレーム以降は初期配置を作り直さない。
	dockLayoutInitialized_ = true;
#endif // USE_IMGUI
}

void ImGuiManager::LoadGizmoIcons() {
#ifdef USE_IMGUI
	if (gizmoIconsLoaded_) {
		return;
	}

	std::filesystem::path imageDirectory = projectWindow_.GetProjectRoot() / "KujakuEngine" / "resources" / "images";
	TextureManager* textureManager = TextureManager::GetInstance();

	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_translate.png").string(), gizmoTranslateIconIndex_);
	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_rotate.png").string(), gizmoRotateIconIndex_);
	textureManager->TryLoadTexture((imageDirectory / "icon_guizmo_scale.png").string(), gizmoScaleIconIndex_);

	gizmoIconsLoaded_ = true;
#endif // USE_IMGUI
}

bool ImGuiManager::DrawGizmoModeButton(const char* id, const char* fallbackLabel, uint32_t textureIndex, TransformGizmoOperation operation, const char* tooltip) {
#ifdef USE_IMGUI
	bool selected = gizmoOperation_ == operation;
	if (selected) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}

	bool clicked = false;
	if (textureIndex != 0) {
		D3D12_GPU_DESCRIPTOR_HANDLE handle = TextureManager::GetInstance()->GetSrvHandle(textureIndex);
		clicked = ImGui::ImageButton(id, static_cast<ImTextureID>(handle.ptr), ImVec2(22.0f, 22.0f));
	} else {
		clicked = ImGui::Button(fallbackLabel, ImVec2(32.0f, 32.0f));
	}

	if (selected) {
		ImGui::PopStyleColor();
	}

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("%s", tooltip);
	}

	if (clicked) {
		gizmoOperation_ = operation;
		return true;
	}
#else
	(void)id;
	(void)fallbackLabel;
	(void)textureIndex;
	(void)operation;
	(void)tooltip;
#endif // USE_IMGUI
	return false;
}

void ImGuiManager::DrawGizmoToolbar() {
#ifdef USE_IMGUI
	LoadGizmoIcons();

	DrawGizmoModeButton("##GizmoTranslate", "T", gizmoTranslateIconIndex_, TransformGizmoOperation::Translate, "Translate");
	ImGui::SameLine();
	DrawGizmoModeButton("##GizmoRotate", "R", gizmoRotateIconIndex_, TransformGizmoOperation::Rotate, "Rotate");
	ImGui::SameLine();
	DrawGizmoModeButton("##GizmoScale", "S", gizmoScaleIconIndex_, TransformGizmoOperation::Scale, "Scale");
#endif // USE_IMGUI
}

void ImGuiManager::DrawTransformGizmo(const ImVec2& imagePosition, const ImVec2& imageSize) {
#ifdef USE_IMGUI
	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		return;
	}

	Camera* camera = scene->GetEditorCamera();
	if (!camera) {
		return;
	}

	GameObject* selectedObject = EditorSelection::GetInstance()->GetSelectedGameObject();
	if (!selectedObject || !selectedObject->IsActive()) {
		return;
	}

	TransformComponent* transformComponent = selectedObject->GetTransformComponent();
	if (!transformComponent) {
		return;
	}

	WorldTransform& transform = transformComponent->GetTransform();
	Matrix4x4 gizmoMatrix = MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(imagePosition.x, imagePosition.y, imageSize.x, imageSize.y);

	ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
	if (gizmoOperation_ == TransformGizmoOperation::Rotate) {
		operation = ImGuizmo::ROTATE;
	} else if (gizmoOperation_ == TransformGizmoOperation::Scale) {
		operation = ImGuizmo::SCALE;
	}

	ImGuizmo::SetGizmoSizeClipSpace(0.2f);

	if (!ImGuizmo::Manipulate(MatrixData(camera->matView), MatrixData(camera->matProjection), operation, ImGuizmo::LOCAL, MatrixData(gizmoMatrix))) {
		return;
	}

	float translation[3]{};
	float rotationDegrees[3]{};
	float scale[3]{};
	ImGuizmo::DecomposeMatrixToComponents(MatrixData(gizmoMatrix), translation, rotationDegrees, scale);

	transform.translation_ = {translation[0], translation[1], translation[2]};
	transform.rotation_ = {
	    rotationDegrees[0] * kDegreesToRadians,
	    rotationDegrees[1] * kDegreesToRadians,
	    rotationDegrees[2] * kDegreesToRadians,
	};
	transform.scale_ = {scale[0], scale[1], scale[2]};

	if (!std::isfinite(transform.scale_.x) || transform.scale_.x < 0.001f) {
		transform.scale_.x = 0.001f;
	}
	if (!std::isfinite(transform.scale_.y) || transform.scale_.y < 0.001f) {
		transform.scale_.y = 0.001f;
	}
	if (!std::isfinite(transform.scale_.z) || transform.scale_.z < 0.001f) {
		transform.scale_.z = 0.001f;
	}
#else
	(void)imagePosition;
	(void)imageSize;
#endif // USE_IMGUI
}

void ImGuiManager::HandleGameWindowObjectSelection(const ImVec2& imagePosition, const ImVec2& imageSize) {
#ifdef USE_IMGUI
	if (EditorApplication::GetInstance()->IsPlaying()) {
		return;
	}
	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
		return;
	}
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		return;
	}
	if (ImGuizmo::IsUsing() || ImGuizmo::IsOver()) {
		return;
	}

	ImVec2 mousePosition = ImGui::GetIO().MousePos;
	if (mousePosition.x < imagePosition.x) {
		return;
	}
	if (mousePosition.y < imagePosition.y) {
		return;
	}
	if (mousePosition.x > imagePosition.x + imageSize.x) {
		return;
	}
	if (mousePosition.y > imagePosition.y + imageSize.y) {
		return;
	}

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		return;
	}

	Camera* camera = scene->GetEditorCamera();
	if (!camera) {
		return;
	}

	GameObject* pickedObject = PickGameObjectInGameWindow(mousePosition, imagePosition, imageSize, *camera);
	if (pickedObject) {
		EditorSelection::GetInstance()->SetSelectedGameObject(pickedObject);
	}
#else
	(void)imagePosition;
	(void)imageSize;
#endif // USE_IMGUI
}

GameObject* ImGuiManager::PickGameObjectInGameWindow(const ImVec2& mousePosition, const ImVec2& imagePosition, const ImVec2& imageSize, const Camera& camera) {
	if (imageSize.x <= 0.0f || imageSize.y <= 0.0f) {
		return nullptr;
	}

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		return nullptr;
	}

	float localX = mousePosition.x - imagePosition.x;
	float localY = mousePosition.y - imagePosition.y;
	float ndcX = localX / imageSize.x * 2.0f - 1.0f;
	float ndcY = 1.0f - localY / imageSize.y * 2.0f;

	Matrix4x4 inverseViewProjection = Inverse(camera.matView * camera.matProjection);
	Vector3 nearPoint = Transform({ndcX, ndcY, 0.0f}, inverseViewProjection);
	Vector3 farPoint = Transform({ndcX, ndcY, 1.0f}, inverseViewProjection);

	PickRay worldRay{};
	worldRay.origin = nearPoint;
	worldRay.direction = Normalize(farPoint - nearPoint);
	if (Length(worldRay.direction) == 0.0f) {
		return nullptr;
	}

	GameObject* nearestObject = nullptr;
	float nearestDistance = (std::numeric_limits<float>::max)();

	for (const std::unique_ptr<GameObject>& gameObject : scene->GetGameObjects()) {
		if (!gameObject || !gameObject->IsActive()) {
			continue;
		}

		ModelRendererComponent* renderer = nullptr;
		if (!HasPickableRenderer(*gameObject, renderer)) {
			continue;
		}

		Vector3 localMin{};
		Vector3 localMax{};
		if (!GetModelLocalBounds(*renderer->GetModel(), localMin, localMax)) {
			continue;
		}

		const WorldTransform& transform = gameObject->GetTransform();
		Matrix4x4 worldMatrix = renderer->GetModel()->GetRootLocalMatrix() * MakeAffineMatrix(transform.scale_, transform.rotation_, transform.translation_);
		Matrix4x4 inverseWorld = Inverse(worldMatrix);

		PickRay localRay{};
		localRay.origin = Transform(worldRay.origin, inverseWorld);
		localRay.direction = TransformNormal(worldRay.direction, inverseWorld);

		float hitDistance = 0.0f;
		if (!IntersectRayAabb(localRay, localMin, localMax, hitDistance)) {
			continue;
		}

		if (hitDistance < nearestDistance) {
			nearestDistance = hitDistance;
			nearestObject = gameObject.get();
		}
	}

	return nearestObject;
}

void ImGuiManager::DrawGameWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Game");
	DrawGizmoToolbar();
	ImGui::Separator();

	// DockされたGameウィンドウの内側サイズを取得する。タブや枠の分を除いた描画可能領域。
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	// Imageに0以下のサイズを渡さないように最低サイズを保証する。
	if (contentSize.x < 1.0f) {
		contentSize.x = 1.0f;
	}
	if (contentSize.y < 1.0f) {
		contentSize.y = 1.0f;
	}

	// DirectXCommonが作ったGame用RenderTargetのSRVをImGuiへ渡す。
	// 描画本体はEditorApplicationでBeginGameRenderからEndGameRenderの間に行われる。
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	D3D12_GPU_DESCRIPTOR_HANDLE handle = dxCommon->GetGameRenderSrvHandle();

	float gameAspect = 16.0f / 9.0f;
	if (dxCommon->GetGameRenderHeight() > 0) {
		gameAspect = static_cast<float>(dxCommon->GetGameRenderWidth()) / static_cast<float>(dxCommon->GetGameRenderHeight());
	}

	ImVec2 imageSize = contentSize;
	ImVec2 imageOffset = {0.0f, 0.0f};
	float contentAspect = contentSize.x / contentSize.y;
	if (contentAspect > gameAspect) {
		imageSize.y = contentSize.y;
		imageSize.x = imageSize.y * gameAspect;
		imageOffset.x = (contentSize.x - imageSize.x) * 0.5f;
	} else {
		imageSize.x = contentSize.x;
		imageSize.y = imageSize.x / gameAspect;
		imageOffset.y = (contentSize.y - imageSize.y) * 0.5f;
	}

	ImVec2 contentPosition = ImGui::GetCursorScreenPos();
	ImVec2 contentEnd = {contentPosition.x + contentSize.x, contentPosition.y + contentSize.y};
	ImVec2 imagePosition = {contentPosition.x + imageOffset.x, contentPosition.y + imageOffset.y};
	ImVec2 imageEnd = {imagePosition.x + imageSize.x, imagePosition.y + imageSize.y};

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(contentPosition, contentEnd, IM_COL32(0, 0, 0, 255));
	drawList->AddImage(static_cast<ImTextureID>(handle.ptr), imagePosition, imageEnd, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
	ImGui::Dummy(contentSize);
	DrawTransformGizmo(imagePosition, imageSize);
	HandleGameWindowObjectSelection(imagePosition, imageSize);
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawHierarchyWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Hierarchy");

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		EditorSelection::GetInstance()->Clear();
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	GameObject* selectedObject = EditorSelection::GetInstance()->GetSelectedGameObject();
	bool selectedObjectExists = false;

	for (const std::unique_ptr<GameObject>& object : scene->GetGameObjects()) {
		if (!object) {
			continue;
		}

		GameObject* raw = object.get();
		if (raw == selectedObject) {
			selectedObjectExists = true;
		}

		std::string displayName = raw->GetName();
		if (!raw->IsActive()) {
			displayName = "[Inactive] " + displayName;
		}

		ImGui::PushID(raw);
		if (!raw->IsActive()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		}

		bool isSelected = selectedObject == raw;
		if (ImGui::Selectable(displayName.c_str(), isSelected)) {
			EditorSelection::GetInstance()->SetSelectedGameObject(raw);
		}

		if (!raw->IsActive()) {
			ImGui::PopStyleColor();
		}
		ImGui::PopID();
	}

	if (ImGui::BeginPopupContextWindow("HierarchyCreateMenu", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("Entity")) {
			GameObject* created = scene->CreateEditorEntity();
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}

		if (ImGui::MenuItem("Cube")) {
			GameObject* created = scene->CreateEditorCube();
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}

		if (ImGui::MenuItem("Sphere")) {
			GameObject* created = scene->CreateEditorSphere();
			EditorSelection::GetInstance()->SetSelectedGameObject(created);
		}

		ImGui::EndPopup();
	}

	if (selectedObject && !selectedObjectExists) {
		EditorSelection::GetInstance()->Clear();
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawInspectorWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Inspector");

	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		EditorSelection::GetInstance()->Clear();
		ImGui::TextDisabled("No Scene.");
		ImGui::End();
		return;
	}

	GameObject* selected = EditorSelection::GetInstance()->GetSelectedGameObject();
	if (!selected) {
		ImGui::TextDisabled("No GameObject selected.");
		ImGui::End();
		return;
	}

	std::array<char, 128> nameBuffer{};
	std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", selected->GetName().c_str());
	if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
		selected->SetName(nameBuffer.data());
	}

	bool active = selected->IsActive();
	if (ImGui::Checkbox("Active", &active)) {
		selected->SetActive(active);
	}

	ImGui::Separator();
	std::vector<std::unique_ptr<Component>>& components = selected->GetComponents();
	if (components.empty()) {
		ImGui::TextDisabled("No Components.");
	}

	Component* removeTarget = nullptr;
	for (const std::unique_ptr<Component>& component : components) {
		if (!component) {
			continue;
		}

		ImGui::PushID(component.get());
		if (ImGui::CollapsingHeader(component->GetTypeName(), ImGuiTreeNodeFlags_DefaultOpen)) {
			bool enabled = component->IsEnabled();
			if (ImGui::Checkbox("Enabled", &enabled)) {
				component->SetEnabled(enabled);
			}

			component->DrawInspector();

			if (component->CanRemove()) {
				if (ImGui::Button("Remove Component")) {
					removeTarget = component.get();
				}
			} else {
				ImGui::TextDisabled("Required Component");
			}
		}
		ImGui::PopID();
	}

	if (removeTarget) {
		selected->RemoveComponent(removeTarget);
	}

	ImGui::Separator();
	if (ImGui::Button("Add Component")) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup")) {
		const std::vector<std::string>& typeNames = ComponentFactory::GetInstance().GetRegisteredTypeNames();
		if (typeNames.empty()) {
			ImGui::TextDisabled("No registered Components.");
		}

		for (const std::string& typeName : typeNames) {
			if (ImGui::MenuItem(typeName.c_str())) {
				Component* added = selected->AddComponent(ComponentFactory::GetInstance().Create(typeName));
				scene->OnEditorComponentAdded(selected, added);
			}
		}

		ImGui::EndPopup();
	}

	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawConsoleWindow() {
#ifdef USE_IMGUI
	ImGui::Begin("Console");
	// ログとは別に、現在モードを常に先頭へ表示する。
	if (EditorApplication::GetInstance()->IsPlaying()) {
		ImGui::TextUnformatted("Editor Mode: Play");
	} else {
		ImGui::TextUnformatted("Editor Mode: Edit");
	}
	ImGui::Separator();
	for (const std::string& log : consoleLogs_) {
		ImGui::TextUnformatted(log.c_str());
	}
	// Consoleが末尾までスクロールされている場合は、新しいログへ追従する。
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::End();
#endif // USE_IMGUI
}

void ImGuiManager::DrawProjectWindow() {
#ifdef USE_IMGUI
	// ProjectDir以下のフォルダ/ファイルを閲覧するProject Windowを描画する。
	projectWindow_.Draw();
#endif // USE_IMGUI
}

void ImGuiManager::HandleEditorShortcuts() {
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
		ExportCurrentSceneJson();
	}
#endif // USE_IMGUI
}

void ImGuiManager::ExportCurrentSceneJson() {
	Scene* scene = EditorApplication::GetInstance()->GetCurrentScene();
	if (!scene) {
		AddConsoleLog("[Editor] Scene JSON export failed: No Scene.");
		return;
	}

	SceneJsonExporter::ExportResult exportResult = SceneJsonExporter::ExportScene(*scene, projectWindow_.GetProjectRoot());
	if (exportResult.succeeded) {
		AddConsoleLog("[Editor] Scene JSON exported: " + exportResult.outputDirectory.string());
		projectWindow_.Refresh();
		return;
	}

	AddConsoleLog("[Editor] Scene JSON export failed: " + exportResult.message);
}

void ImGuiManager::DrawEditor() {
#ifdef USE_IMGUI
	// Editor UIを構成する各ウィンドウを毎フレーム描画する。
	// 実際の配置はDockSpace側が管理するため、ここでは各ウィンドウを出すだけでよい。
	HandleEditorShortcuts();
	DrawDockSpace();
	DrawGameWindow();
	DrawHierarchyWindow();
	DrawInspectorWindow();
	// ProjectはDockBuilderでHierarchyとInspectorの間に初期配置される。
	DrawProjectWindow();
	DrawConsoleWindow();
#endif // USE_IMGUI
}

void ImGuiManager::Begin() {
#ifdef USE_IMGUI

	// エンジンのフレーム先頭処理に対応
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
#endif // USE_IMGUI
}

void ImGuiManager::End() {
#ifdef USE_IMGUI

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	// Project Windowで要求されたモデルサムネイルを、ImGui本体の描画前にOffscreenへ描く。
	projectWindow_.RenderModelPreviews();
	ImGui::Render();
	// ImGuiで作ったUIをDirectX12のCommandListに描画命令として積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());
#endif // USE_IMGUI
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // USE_IMGUI
}

} // namespace KujakuEngine
