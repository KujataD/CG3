#include <KujakuEngine.h>

#include <cassert>
#include <fstream>
#include <memory>

using namespace KujakuEngine;

// Play開始時の編集状態を一時保存するための簡易スナップショット。
// 今回は完全なScene Cloneを作らず、まずは対象Transformだけ復元できるようにしている。
struct TransformSnapshot {
	Vector3 scale;
	Vector3 rotation;
	Vector3 translation;
};

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	KujakuEngine::Initialize(L"LC2B_04_オオツカ_ダイチ_AL3");

	// カメラ
	// ------------------------------------------
	Camera camera;
	camera.Initialize();
	camera.translation_ = {0.0f, 0.0f, -20.0f};
	camera.UpdateMatrix();

	DebugCamera debugCamera;
	debugCamera.Initialize(camera.rotation_, camera.translation_);

	DirectionalLight::GetInstance()->GetData().intensity = 0.0f;

	PointLight::GetInstance()->Reset();
	SpotLight::GetInstance()->Reset();

	SpotLightData spotLight;
	spotLight.position = {0.0f, 5.0f, 0.0f};
	spotLight.distance = 50.0f;
	spotLight.direction = Normalize({0.0f, -1.0f, 0.0f});
	spotLight.intensity = 1.0f;
	spotLight.decay = 1.0f;
	spotLight.cosAngle = std::cos(std::numbers::pi_v<float> / 2.0f);
	spotLight.cosFalloffStart = std::cos(std::numbers::pi_v<float> / 3.0f);

	// モンスターボール
	// ------------------------------------------
	Model* modelBall = Model::CreateSphere("resources/monsterBall.png", ShaderModel::kBlingPhongReflection);
	WorldTransform ballTransform;
	ballTransform.Initialize();
	ballTransform.translation_ = {0.0f, -2.0f, 0.0f};
	ballTransform.rotation_.x = std::numbers::pi_v<float> * -0.5f;
	// 起動時の状態をEdit状態として保存しておく。
	// 将来的にはこの考え方をScene全体のCloneに置き換える。
	TransformSnapshot editBallSnapshot{ballTransform.scale_, ballTransform.rotation_, ballTransform.translation_};
	// 前フレームのPlay状態。EditからPlay、PlayからEditへの切り替わりを検出する。
	bool wasPlaying = false;

	// テレイン
	// ------------------------------------------
	Model* modelTerrain = Model::CreateFromGlTF("plane", ShaderModel::kBlingPhongReflection);

	// ゲームループ
	while (KujakuEngine::Update()) {

		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

#ifdef USE_IMGUI
		// DockSpaceと各Editorウィンドウを描画する。
		// ボタン操作によってImGuiManager内のEditorModeが更新される。
		ImGuiManager::GetInstance()->DrawEditor();
		// 現在Play中かどうかを取得する。
		// この値でゲームロジックを進めるか止めるかを決める。
		bool isPlaying = ImGuiManager::GetInstance()->IsPlaying();
#else
		// ImGuiを使わないビルドでは従来通り常にゲームを動かす。
		bool isPlaying = true;
#endif // USE_IMGUI

		// EditからPlayへ切り替わった瞬間に、編集状態のTransformを保存する。
		// 今回は暫定実装なので、Scene全体ではなくボールだけを対象にしている。
		if (!wasPlaying && isPlaying) {
			editBallSnapshot.scale = ballTransform.scale_;
			editBallSnapshot.rotation = ballTransform.rotation_;
			editBallSnapshot.translation = ballTransform.translation_;
		}

		// PlayからEditへ戻った瞬間に、保存していた編集状態へ戻す。
		// 完全なScene Cloneを導入するまでの簡易的な復元処理。
		if (wasPlaying && !isPlaying) {
			ballTransform.scale_ = editBallSnapshot.scale;
			ballTransform.rotation_ = editBallSnapshot.rotation;
			ballTransform.translation_ = editBallSnapshot.translation;
		}
		// 次フレームで状態遷移を判定できるよう、現在の状態を保存する。
		wasPlaying = isPlaying;

		// カメラ更新
		// --------------------------------------
		debugCamera.Update();
		camera.translation_ = debugCamera.translation_;
		camera.rotation_ = debugCamera.rotation_;
		camera.UpdateMatrix();

		// ボール
		// --------------------------------------
		// Play中だけゲームロジックを進める。
		// Edit中はUpdateを止める仕様なので、回転値を変更しない。
		if (isPlaying) {
			ballTransform.rotation_.x += 0.02f;
		}
		// 描画には常に最新の行列が必要なので、Edit中でも行列更新は行う。
		// これにより、カメラ移動など描画に必要な最低限の反映は保てる。
		ballTransform.UpdateMatrix(camera);

		// ライト更新
		spotLight.direction = Normalize(spotLight.direction);
		SpotLight::GetInstance()->SetLight(&spotLight);
		

		///
		/// ↑↑↑ 更新処理ここまで ↑↑↑
		///

		PreDraw();
#ifdef USE_IMGUI
		// ここから既存のゲーム描画をGameウィンドウ用RenderTargetへ流す。
		// SwapChainではなくOffscreenへ描き、後でImGui::Imageとして表示する。
		DirectXCommon::GetInstance()->BeginGameRender();
#endif // USE_IMGUI

		///
		/// ↓↓↓ 描画処理ここから ↓↓↓
		///

		ParticleModel::PreDraw();
		ParticleModel::PostDraw();

		Model::PreDraw();
		modelBall->Draw(ballTransform, camera);
		modelTerrain->Draw(ballTransform, camera);
		Model::PostDraw();

		Sprite::PreDraw();
		Sprite::PostDraw();

		///
		/// ↑↑↑ 描画処理ここまで ↑↑↑
		///
#ifdef USE_IMGUI
		// Gameウィンドウ用RenderTargetへの描画を終え、ImGuiから読める状態に戻す。
		// この後のPostDraw内でImGui自体はSwapChainバックバッファへ描かれる。
		DirectXCommon::GetInstance()->EndGameRender();
#endif // USE_IMGUI
		PostDraw();
	}

	// エンジンの終了処理
	KujakuEngine::Finalize();

	return 0;
}
