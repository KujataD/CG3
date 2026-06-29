#include "Component.h"
#include <ostream>
#include <string>

namespace KujakuEngine {

Component::~Component() = default;

void Component::WriteJson(std::ostream& os, int indent) const {
	const std::string padding(static_cast<size_t>(indent), ' ');
	os << padding << "{\n";
	os << padding << "  \"type\": \"" << GetTypeName() << "\",\n";
	os << padding << "  \"enabled\": ";
	if (IsEnabled()) {
		os << "true\n";
	} else {
		os << "false\n";
	}
	os << padding << "}";
}

} // namespace KujakuEngine
