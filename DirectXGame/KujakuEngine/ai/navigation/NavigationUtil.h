#pragma once
#include <cmath>
#include "Grid.h"

namespace KujakuEngine {

namespace NavigationUtil {

enum class HeuristicType {
	kManhattan,
	kEuclidean,
};

float Heuristic(GridIndex a, GridIndex b, HeuristicType heuristicType = HeuristicType::kManhattan);

} // namespace NavigationUtil
} // namespace KujakuEngine