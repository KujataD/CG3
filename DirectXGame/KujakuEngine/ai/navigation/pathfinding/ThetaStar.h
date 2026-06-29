#pragma once
#include "../../navigation/NavigationUtil.h"
#include "../../navigation/Grid.h"
#include "../../navigation/SearchNode.h"
#include <numbers>
#include <vector>

namespace KujakuEngine{
class ThetaStar {
public:
	enum class HeuristicAlgorithm {
		kManhattan,
		kEuclidean,
	};

public:
	ThetaStar() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="mapChipField"></param>
	void Init(const Grid* grid);

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
	void SetAgentRadius(float radius) { agentRadius_ = radius; }

private:
	const Grid* grid_;
	HeuristicAlgorithm heuristicAlgorithm_ = HeuristicAlgorithm::kEuclidean;
	float agentRadius_ = 0.3f;

private:
	/// <summary>
	/// 歩ける場所の設定
	/// </summary>
	/// <param name="x"></param>
	/// <param name="z"></param>
	/// <returns></returns>
	bool IsWalkable(int32_t x, int32_t y) const;

	bool HasLineOfSight(const SearchNode* a, const SearchNode* b);

	float Distance(const SearchNode* a, const SearchNode* b);

	bool IsSegmentClearOfBlockedCell(float startX, float startY, float endX, float endY, int32_t cellX, int32_t cellY) const;
	bool SegmentIntersectsAABB(float startX, float startY, float endX, float endY, float minX, float minY, float maxX, float maxY) const;

};
}
