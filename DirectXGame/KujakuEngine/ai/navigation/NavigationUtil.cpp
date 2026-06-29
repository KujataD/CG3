#include "NavigationUtil.h"

namespace KujakuEngine {

namespace NavigationUtil {

float Heuristic(GridIndex a, GridIndex b, HeuristicType heuristicType) {
	float baX = std::abs(static_cast<float>(a.x) - static_cast<float>(b.x));
	float baY = std::abs(static_cast<float>(a.y) - static_cast<float>(b.y));

	switch (heuristicType) {
	case HeuristicType::kManhattan:
		return baX + baY;

	case HeuristicType::kEuclidean:
		return std::sqrt(baX * baX + baY * baY);
	}
	return baX + baY;
}

} // namespace NavigationUtil

} // namespace KujakuEngine