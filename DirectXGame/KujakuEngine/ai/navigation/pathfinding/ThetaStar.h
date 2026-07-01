#pragma once
#include "../../navigation/NavigationUtil.h"
#include "../../navigation/Grid.h"
#include "../../navigation/SearchNode.h"
#include <numbers>
#include <vector>

namespace KujakuEngine{
/// <summary>
/// ThetaStarクラスを表します。
/// </summary>
class ThetaStar {
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
	/// ThetaStarを実行します。
	/// </summary>
	ThetaStar() = default;

	/// <param name="mapChipField"></param>
	/// <summary>
	/// Initを実行します。
	/// </summary>
	void Init(const Grid* grid);

	/// <param name="start"></param>
	/// <param name="goal"></param>
	/// <returns></returns>
	/// <summary>
	/// Pathを検索します。
	/// </summary>
	std::vector<GridIndex> FindPath(GridIndex start, GridIndex goal);

	/// <param name="heuristicAlgorithm"></param>
	void SetHeuristicAlgorithm(HeuristicAlgorithm heuristicAlgorithm) { heuristicAlgorithm_ = heuristicAlgorithm; }
	void SetAgentRadius(float radius) { agentRadius_ = radius; }

private:
	const Grid* grid_;
	HeuristicAlgorithm heuristicAlgorithm_ = HeuristicAlgorithm::kEuclidean;
	float agentRadius_ = 0.3f;

private:
	/// <param name="x"></param>
	/// <param name="z"></param>
	/// <returns></returns>
	/// <summary>
	/// Walkableかどうかを返します。
	/// </summary>
	bool IsWalkable(int32_t x, int32_t y) const;

	/// <summary>
	/// LineOfSightを持つかどうかを返します。
	/// </summary>
	bool HasLineOfSight(const SearchNode* a, const SearchNode* b);

	/// <summary>
	/// Distanceを実行します。
	/// </summary>
	float Distance(const SearchNode* a, const SearchNode* b);

	/// <summary>
	/// SegmentClearOfBlockedCellかどうかを返します。
	/// </summary>
	bool IsSegmentClearOfBlockedCell(float startX, float startY, float endX, float endY, int32_t cellX, int32_t cellY) const;
	/// <summary>
	/// SegmentIntersectsAABBを実行します。
	/// </summary>
	bool SegmentIntersectsAABB(float startX, float startY, float endX, float endY, float minX, float minY, float maxX, float maxY) const;

};
}
