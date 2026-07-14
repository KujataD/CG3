#pragma once
#include <KujakuEngine.h>

class Player : public KujakuEngine::Component {
public:
	const char* GetTypeName() const override { return "Player"; }

	void Initialize() override;
	void Update() override;

private:
	KUJAKU_SERIALIZED_FIELDS_BEGIN() {
		KUJAKU_REGISTER_FLOAT(speed_, 0.001f, 0.0f, 0.0f);
	}

	KUJAKU_FIELD_FLOAT(speed_, 0.2f);

};
