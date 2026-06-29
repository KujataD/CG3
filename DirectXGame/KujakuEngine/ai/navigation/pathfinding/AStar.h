#pragma once
#include "../Grid.h"
#include "../SearchNode.h"
#include "../NavigationUtil.h"
#include <numbers>
#include <vector>

namespace KujakuEngine{

class AStar {
public:
	enum class HeuristicAlgorithm {
		kManhattan,
		kEuclidean,
	};

public:
	AStar() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="mapChipField"></param>
	void Init(const Grid* mapChipField);

	/// <summary>
	/// 探索関数
	/// </summary>
	/// <param name="start"></param>
	/// <param name="goal"></param>
	/// <returns></returns>
	std::vector<GridIndex> FindPath(GridIndex start, GridIndex goal);

	/// <summary>
	/// ヒューリスティックの距離アルゴリズムを変更する
	/// </summary>
	/// <param name="heuristicAlgorithm"></param>
	void SetHeuristicAlgorithm(HeuristicAlgorithm heuristicAlgorithm) { heuristicAlgorithm_ = heuristicAlgorithm; }

private:
	const Grid* grid_;
	HeuristicAlgorithm heuristicAlgorithm_ = HeuristicAlgorithm::kEuclidean;

private:
	/// <summary>
	/// 歩ける場所の設定
	/// </summary>
	/// <param name="x"></param>
	/// <param name="z"></param>
	/// <returns></returns>
	bool IsWalkable(uint32_t x, uint32_t y) const { return grid_->IsWalkable(x, y); }

};
}