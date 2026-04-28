#include <KujakuEngine.h>
#include <cassert>
#include <fstream>
#include <memory>

#define DT 1.0f / 60.0f;

using namespace KujakuEngine;

struct Particle {
	WorldTransform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

Particle* MakeNewParticle();

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// エンジン初期化
	KujakuEngine::Initialize(L"LC2B_04_オオツカ_ダイチ_AL3", true);

	// ImGuiManagerインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	// カメラ
	// ------------------------------------------
	Camera camera;
	camera.Initialize();
	camera.translation_ = {0.0f, 0.0f, -20.0f};
	camera.UpdateMatrix();

	DebugCamera debugCamera;
	debugCamera.Initialize(camera.rotation_, camera.translation_);

	// パーティクル
	// ------------------------------------------
	ParticleModel* particleModel = ParticleModel::CreatePlane("resources/circle.png", true);
	particleModel->Initialize();
	//particleModel->SetBlendMode(BlendMode::kAdd);

	std::vector<Particle*> particles;
	const uint32_t kNumParticle = 10u;

	for (uint32_t i = 0; i < kNumParticle; i++) {
		particles.push_back(MakeNewParticle());
	}

	// ゲームループ
	while (KujakuEngine::Update()) {
		Input::Update();

		imguiManager->Begin();

		///
		/// ↓↓↓ 更新処理ここから ↓↓↓
		///

		// カメラ
		debugCamera.Update();
		camera.translation_ = debugCamera.translation_;
		camera.rotation_ = debugCamera.rotation_;
		camera.UpdateMatrix();

		// パーティクル処理
		for (Particle* particle : particles) {
			particle->currentTime += DT;
			if (particle->lifeTime <= particle->currentTime) {
				continue;
			}
			float alpha = 1.0f - (particle->currentTime / particle->lifeTime);
			particle->color.w = alpha;
			particle->transform.translation_ += particle->velocity * DT;
			particle->transform.UpdateBillboardMatrix(camera);
			// InstancingModelに追加
			particleModel->AddInstanceParticle(particle->transform.GetMatrixData(camera), particle->color);
		}
		particleModel->UpdateBuffer();

#ifdef USE_IMGUI
		ImGui::Begin("LightManager");
		auto& light = DirectionalLight::GetInstance()->GetData();
		ImGui::ColorEdit3("Light Color", &light.color.x);
		ImGui::SliderFloat3("Direction", &light.direction.x, -1.0f, 1.0f);
		ImGui::DragFloat("Intensity", &light.intensity, 0.01f);
		if (ImGui::Button("Particle Make")) {
			for (uint32_t i = 0; i < kNumParticle; i++) {
				particles.push_back(MakeNewParticle());
			}
		}
		ImGui::End();
#endif // USE_IMGUI

		///
		/// ↑↑↑ 更新処理ここまで ↑↑↑
		///

		PreDraw();

		///
		/// ↓↓↓ 描画処理ここから ↓↓↓
		///

		ParticleModel::PreDraw();
		particleModel->Draw(camera);
		ParticleModel::PostDraw();

		Sprite::PreDraw();
		Sprite::PostDraw();

		///
		/// ↑↑↑ 描画処理ここまで ↑↑↑
		///
		PostDraw();
	}

	// エンジンの終了処理
	KujakuEngine::Finalize();

	return 0;
}

Particle* MakeNewParticle() {
	Particle* particle = new Particle;
	particle->transform.Initialize();
	particle->transform.translation_ = {Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f)};
	particle->transform.rotation_.y = std::numbers::pi_v<float>;
	particle->velocity = {Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f)};
	particle->color = {Random::GetRandom(0.0f, 1.0f), Random::GetRandom(0.0f, 1.0f), Random::GetRandom(0.0f, 1.0f), 1.0f};
	particle->lifeTime = Random::GetRandom(1.0f, 3.0f);
	particle->currentTime = 0.0f;
	return particle;
}
