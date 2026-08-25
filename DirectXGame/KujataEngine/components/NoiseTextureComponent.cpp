#include "NoiseTextureComponent.h"
#include "../assets/NoiseTexture.h"
#include "../scene/GameObject.h"
#include "ModelRendererComponent.h"
#include "ParticleSystemComponent.h"
#include <algorithm>

namespace KujataEngine {

void NoiseTextureComponent::Initialize() { RegenerateAndApply(); }

void NoiseTextureComponent::RegenerateAndApply() {
	if (!GetOwner()) {
		return;
	}

	uint32_t size = static_cast<uint32_t>((std::max)(textureSize_, 1));

	if (kind_ == static_cast<int>(Kind::DustSprite)) {
		uint32_t textureIndex = NoiseTexture::GenerateDustSpriteTexture(size, seedLane_);
		if (ParticleSystemComponent* particles = GetComponent<ParticleSystemComponent>()) {
			particles->SetTextureOverride(textureIndex);
		}
		return;
	}

	uint32_t textureIndex = (kind_ == static_cast<int>(Kind::SoilAlbedo)) ? NoiseTexture::GenerateSoilAlbedoTexture(size, seedLane_)
	                                                                     : NoiseTexture::GenerateRockAlbedoTexture(size, seedLane_);
	if (ModelRendererComponent* renderer = GetComponent<ModelRendererComponent>()) {
		renderer->SetTextureOverride(textureIndex);
		renderer->SetUVTilingOverride(uvTiling_);
	}
}

void NoiseTextureComponent::RegisterInvokableMethods(InvokableMethodRegistry& registry) { registry.Add("Regenerate", [this]() { RegenerateAndApply(); }); }

} // namespace KujataEngine
