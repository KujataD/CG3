#pragma once
#include <cmath>
#include "Grid.h"

namespace KujakuEngine {

namespace NavigationUtil {

/// <summary>
/// HeuristicTypeの種類を表します。
/// </summary>
enum class HeuristicType {
	kManhattan,
	kEuclidean,
};

/// <summary>
/// Heuristicを実行します。
/// </summary>
float Heuristic(GridIndex a, GridIndex b, HeuristicType heuristicType = HeuristicType::kManhattan);

} // namespace NavigationUtil
} // namespace KujakuEngine