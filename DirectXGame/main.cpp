#include <KujakuEngine.h>
#include <cassert>
#include <fstream>
#include <memory>

#define DT 1.0f / 60.0f;

using namespace KujakuEngine;

struct Emitter {

	WorldTransform transform; // !< エミッタのTransform
	uint32_t count;           // !< 発生数
	float frequency;          // !< 発生頻度
	float frequencyTime;      // !< 頻度用時刻
};

struct Particle {
	WorldTransform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

Particle* MakeNewParticle(const Vector3& translate);

std::list<Particle*> Emit(const Emitter& emitter);

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
	particleModel->SetBlendMode(BlendMode::kAdd);

	std::list<Particle*> particles;
	const uint32_t kNumParticle = 10u;

	// エミッター
	// ------------------------------------------
	Emitter emitter{};
	emitter.transform.translation_ = {0.0f, 0.0f, 0.0f};
	emitter.transform.rotation_ = {0.0f, 0.0f, 0.0f};
	emitter.transform.scale_ = {0.0f, 0.0f, 0.0f};
	emitter.count = 3;
	emitter.frequency = 0.5f;     // 0.5秒ごとに発生
	emitter.frequencyTime = 0.0f; // 発生頻度用の時刻、0で初期化

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

		// エミッター処理
		emitter.frequencyTime += DT;                          // 時刻を進める
		if (emitter.frequency <= emitter.frequencyTime) {     // 頻度より大きいなら発生
			particles.splice(particles.end(), Emit(emitter)); // 発生処理
			emitter.frequencyTime -= emitter.frequency;       // 余計に過ぎた時間も加味して頻度計算する
		}

		// パーティクル処理
		for (std::list<Particle*>::iterator particleIterator = particles.begin(); particleIterator != particles.end();) {
			(*particleIterator)->currentTime += DT;
			if ((*particleIterator)->lifeTime <= (*particleIterator)->currentTime) {
				delete *particleIterator;
				particleIterator = particles.erase(particleIterator);
				continue;
			}
			float alpha = 1.0f - ((*particleIterator)->currentTime / (*particleIterator)->lifeTime);
			(*particleIterator)->color.w = alpha;
			(*particleIterator)->transform.translation_ += (*particleIterator)->velocity * DT;
			(*particleIterator)->transform.UpdateBillboardMatrix(camera);
			// InstancingModelに追加
			particleModel->AddInstanceParticle((*particleIterator)->transform.GetMatrixData(camera), (*particleIterator)->color);
			++particleIterator;
		}
		particleModel->UpdateBuffer();

#ifdef USE_IMGUI
		ImGui::Begin("LightManager");
		auto& light = DirectionalLight::GetInstance()->GetData();
		ImGui::ColorEdit3("Light Color", &light.color.x);
		ImGui::SliderFloat3("Direction", &light.direction.x, -1.0f, 1.0f);
		ImGui::DragFloat("Intensity", &light.intensity, 0.01f);
		ImGui::Text("particle.size %d", (int)particles.size());
		ImGui::DragFloat3("EmitterTranslate", &emitter.transform.translation_.x , 0.01f, -100.0f, 100.0f);
		if (ImGui::Button("Particle Make")) {
			particles.splice(particles.end(), Emit(emitter));
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

	delete particleModel;
	particleModel = nullptr;

	for (Particle* particle : particles) {
		delete particle;
	}
	particles.clear();

	// エンジンの終了処理
	KujakuEngine::Finalize();

	return 0;
}

Particle* MakeNewParticle(const Vector3& translate) {
	Particle* particle = new Particle;
	particle->transform.Initialize();
	Vector3 randomTranslation = {Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f)};

	particle->transform.translation_ = translate + randomTranslation;
	particle->transform.rotation_.y = std::numbers::pi_v<float>;
	particle->velocity = {Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f), Random::GetRandom(-1.0f, 1.0f)};
	particle->color = {Random::GetRandom(0.0f, 1.0f), Random::GetRandom(0.0f, 1.0f), Random::GetRandom(0.0f, 1.0f), 1.0f};
	particle->lifeTime = Random::GetRandom(1.0f, 3.0f);
	particle->currentTime = 0.0f;
	return particle;
}

std::list<Particle*> Emit(const Emitter& emitter) {
	std::list<Particle*> particles;
	for (uint32_t count = 0; count < emitter.count; ++count) {
		particles.push_back(MakeNewParticle(emitter.transform.translation_));
	}
	return particles;
}