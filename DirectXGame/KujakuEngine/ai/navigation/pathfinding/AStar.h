#pragma once
#include "../Grid.h"
#include "../SearchNode.h"
#include "../NavigationUtil.h"
#include <numbers>
#include <vector>

namespace KujakuEngine{

/// <summary>
/// AStarクラスを表します。
/// </summary>
class AStar {
public:
	/// <summary>
	/// HeuristicAlgorithmの種類を表します。
	/// </summary>
	enum class HeuristicAlgorithm {
		kManhattan,
		kEuclidean,
	};

public:
	/// <summary>
	/// AStarを実行します。
	/// </summary>
	AStar() = default;

	/// <param name="mapChipField"></param>
	/// <summary>
	/// Initを実行します。
	/// </summary>
	void Init(const Grid* mapChipField);

	/// <param name="start"></param>
	/// <param name="goal"></param>
	/// <returns></returns>
	/// <summary>
	/// Pathを検索します。
	/// </summary>
	std::vector<GridIndex> FindPath(GridIndex start, GridIndex goal);

	/// <param name="heuristicAlgorithm"></param>
	void SetHeuristicAlgorithm(HeuristicAlgorithm heuristicAlgorithm) { heuristicAlgorithm_ = heuristicAlgorithm; }

private:
	const Grid* grid_;
	HeuristicAlgorithm heuristicAlgorithm_ = HeuristicAlgorithm::kEuclidean;

private:
	/// <param name="x"></param>
	/// <param name="z"></param>
	/// <returns></returns>
	/// <summary>
	/// Walkableかどうかを返します。
	/// </summary>
	bool IsWalkable(uint32_t x, uint32_t y) const { return grid_->IsWalkable(x, y); }

};
}