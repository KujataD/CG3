#include "Component.h"
#include <ostream>
#include <sstream>
#include <string>

namespace KujakuEngine {

namespace {

void WriteIndentedJson(std::ostream& os, const nlohmann::json& json, int indent) {
	const std::string padding(static_cast<size_t>(indent), ' ');
	std::istringstream lines(json.dump(2));
	std::string line;
	bool firstLine = true;

	while (std::getline(lines, line)) {
		if (!firstLine) {
			os << "\n";
		}
		os << padding << line;
		firstLine = false;
	}
}

} // namespace

Component::~Component() = default;

bool Component::HasEditorBillboard() const {
	return false;
}

const char* Component::GetEditorBillboardIconName() const {
	return "";
}

float Component::GetEditorBillboardPickRadius() const {
	return 0.5f;
}

void Component::WriteJson(std::ostream& os, int indent) const {
	nlohmann::json properties = nlohmann::json::object();
	WriteJson(properties);

	nlohmann::json componentJson;
	componentJson["type"] = GetTypeName();
	componentJson["enabled"] = IsEnabled();
	componentJson["properties"] = properties;

	WriteIndentedJson(os, componentJson, indent);
}

} // namespace KujakuEngine
