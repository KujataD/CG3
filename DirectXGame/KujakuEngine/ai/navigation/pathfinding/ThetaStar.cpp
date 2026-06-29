#include "ThetaStar.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace KujakuEngine {

// aはbより後回しにすべきかどうか
// 例) a>bの時、小さいbを優先すべきなので、後回しにすべき
struct CompareFCost {
	bool operator()(SearchNode* a, SearchNode* b) const { return a->f > b->f; }
};

void ThetaStar::Init(const Grid* grid) {
	assert(grid);
	grid_ = grid;
}

std::vector<GridIndex> ThetaStar::FindPath(GridIndex start, GridIndex goal) {
	// 返り値
	std::vector<GridIndex> path;

	// 次に調べる候補ノード。優先度付きキューを用いて、自動でコストが低い順になる。
	std::priority_queue<SearchNode*, std::vector<SearchNode*>, CompareFCost> openList;
	// オープンリストに入っている場所のリスト
	std::vector<std::vector<bool>> inOpen(grid_->height, std::vector<bool>(grid_->width, false));

	// 記録してきたすべてのノード
	std::vector<SearchNode*> allPathNodes;

	// すでに調べた場所
	std::vector<std::vector<bool>> closed(grid_->height, std::vector<bool>(grid_->width, false));

	// --- スタートノード ---
	SearchNode* startPathNode = new SearchNode(start.x, start.y);
	startPathNode->g = 0.0f;
	startPathNode->h = NavigationUtil::Heuristic(start, goal);
	startPathNode->f = startPathNode->h;
	startPathNode->parent = nullptr;

	openList.push(startPathNode); // push() で追加
	allPathNodes.push_back(startPathNode);

	// 調べていないノードがなくなるまでループ
	while (!openList.empty()) {

		SearchNode* currentPathNode = openList.top(); // 先頭取得
		openList.pop();                               // 先頭削除

		// --- ゴール判定 ---
		if (currentPathNode->x == static_cast<int32_t>(goal.x) && currentPathNode->y == static_cast<int32_t>(goal.y)) {

			// 親をたどってpath配列に追加していく
			SearchNode* pathNode = currentPathNode;
			while (pathNode) {
				path.push_back({(uint32_t)pathNode->x, (uint32_t)pathNode->y});
				pathNode = pathNode->parent;
			}

			// ゴール→スタートの順なので順序反転する
			std::reverse(path.begin(), path.end());

			// メモリ解放
			for (SearchNode* n : allPathNodes) {
				delete n;
			}

			return path;
		}

		closed[currentPathNode->y][currentPathNode->x] = true;

		// --- 隣接しているノードを調べる ---

		// 上下左右の組み合わせ
		const int dx[8] = {1, -1, 0, 0, 1, -1, 1, -1};
		const int dz[8] = {0, 0, 1, -1, 1, 1, -1, -1};

		// 8方向アルゴリズムの場合は4
		int indexNum = 8;

		// 4方向アルゴリズムの場合は4
		if (heuristicAlgorithm_ == HeuristicAlgorithm::kManhattan) {
			indexNum = 4;
		}

		for (int i = 0; i < indexNum; i++) {

			// 近接マス
			int32_t neighborX = currentPathNode->x + dx[i];
			int32_t neighborZ = currentPathNode->y + dz[i];

			// 範囲チェック
			if (neighborX < 0 || neighborX >= static_cast<int32_t>(grid_->width)) {
				continue;
			}
			if (neighborZ < 0 || neighborZ >= static_cast<int32_t>(grid_->height)) {
				continue;
			}

			// 通行チェック
			if (!IsWalkable(neighborX, neighborZ)) {
				continue;
			}

			// すでに調べていたらスキップ
			if (closed[neighborZ][neighborX]) {
				continue;
			}

			// 隣接ノードを追加する前にチェック
			if (inOpen[neighborZ][neighborX]) {
				continue; // 重複追加を防ぐ
			}

			bool isDiagonal = (i >= 4); // 4番目以降が斜め
			// 斜め移動する際、壁にめり込まないようにする。
			if (isDiagonal) {
				// 右上に進む場合、右が壁でないかどうか
				bool xWalkable = IsWalkable(currentPathNode->x + dx[i], currentPathNode->y);
				// 右上に進む場合、上が壁でないかどうか
				bool zWalkable = IsWalkable(currentPathNode->x, currentPathNode->y + dz[i]);
				if (!xWalkable || !zWalkable) {
					continue;
				}
			}

			// 近接ノード
			SearchNode* neighbor = new SearchNode(neighborX, neighborZ);
			float stepCost;

			// 斜め移動の時のコストはroot2
			if (isDiagonal) {
				stepCost = std::numbers::sqrt2_v<float>;
			} else {
				stepCost = 1.0f;
			}

			// 斜め移動
			if (currentPathNode->parent && HasLineOfSight(currentPathNode->parent, neighbor)) {

				// 親をスキップ
				neighbor->parent = currentPathNode->parent;

				float newG = currentPathNode->parent->g + Distance(currentPathNode->parent, neighbor);

				neighbor->g = newG;

			} else {

				// 通常A*
				neighbor->parent = currentPathNode;

				neighbor->g = currentPathNode->g + stepCost;
			}
			neighbor->h = NavigationUtil::Heuristic({(uint32_t)neighborX, (uint32_t)neighborZ}, goal);
			neighbor->f = neighbor->g + neighbor->h;

			openList.push(neighbor); // push() で追加
			allPathNodes.push_back(neighbor);
		}
	}

	// 見つからなかった場合
	for (SearchNode* n : allPathNodes) {
		delete n;
	}

	return {};
}

bool ThetaStar::IsWalkable(int32_t x, int32_t y) const {
	if (x < 0 || y < 0) {
		return false;
	}

	return grid_->IsWalkable(static_cast<uint32_t>(x), static_cast<uint32_t>(y));
}

bool ThetaStar::HasLineOfSight(const SearchNode* startNode, const SearchNode* endNode) {
	if (!IsWalkable(startNode->x, startNode->y) || !IsWalkable(endNode->x, endNode->y)) {
		return false;
	}

	const float startX = static_cast<float>(startNode->x) + 0.5f;
	const float startY = static_cast<float>(startNode->y) + 0.5f;
	const float endX = static_cast<float>(endNode->x) + 0.5f;
	const float endY = static_cast<float>(endNode->y) + 0.5f;

	const int32_t minCellX = std::max<int32_t>(0, static_cast<int32_t>(std::floor(std::min(startX, endX) - agentRadius_)) - 1);
	const int32_t minCellY = std::max<int32_t>(0, static_cast<int32_t>(std::floor(std::min(startY, endY) - agentRadius_)) - 1);
	const int32_t maxCellX = std::min<int32_t>(static_cast<int32_t>(grid_->width) - 1, static_cast<int32_t>(std::ceil(std::max(startX, endX) + agentRadius_)) + 1);
	const int32_t maxCellY = std::min<int32_t>(static_cast<int32_t>(grid_->height) - 1, static_cast<int32_t>(std::ceil(std::max(startY, endY) + agentRadius_)) + 1);

	for (int32_t y = minCellY; y <= maxCellY; ++y) {
		for (int32_t x = minCellX; x <= maxCellX; ++x) {
			if (IsWalkable(x, y)) {
				continue;
			}

			if (!IsSegmentClearOfBlockedCell(startX, startY, endX, endY, x, y)) {
				return false;
			}
		}
	}

	return true;

	int currentX = static_cast<int>(startNode->x);
	int currentY = static_cast<int>(startNode->y);
	int targetX = static_cast<int>(endNode->x);
	int targetY = static_cast<int>(endNode->y);

	// スタートからゴールまでの差分
	int deltaX = abs(targetX - currentX);
	int deltaY = abs(targetY - currentY);

	// 進行方向
	int stepX = (currentX < targetX) ? 1 : -1;
	int stepY = (currentY < targetY) ? 1 : -1;

	// 誤差値
	int error = deltaX - deltaY;

	while (true) {
		if (!IsWalkable(currentX, currentY)) {
			return false;
		}

		if (currentX == targetX && currentY == targetY) {
			break;
		}

		// 次マスへ進む
		int doubledError = error * 2;
		if (doubledError > -deltaY) {
			error -= deltaY;
			currentX += stepX;
		}

		if (doubledError < deltaX) {
			error += deltaX;
			currentY += stepY;
		}
	}

	return true;
}

bool ThetaStar::IsSegmentClearOfBlockedCell(float startX, float startY, float endX, float endY, int32_t cellX, int32_t cellY) const {
	const float minX = static_cast<float>(cellX) - agentRadius_;
	const float minY = static_cast<float>(cellY) - agentRadius_;
	const float maxX = static_cast<float>(cellX + 1) + agentRadius_;
	const float maxY = static_cast<float>(cellY + 1) + agentRadius_;

	return !SegmentIntersectsAABB(startX, startY, endX, endY, minX, minY, maxX, maxY);
}

bool ThetaStar::SegmentIntersectsAABB(float startX, float startY, float endX, float endY, float minX, float minY, float maxX, float maxY) const {
	const float diffX = endX - startX;
	const float diffY = endY - startY;
	float tMin = 0.0f;
	float tMax = 1.0f;

	auto clip = [](float denominator, float numerator, float& inOutMin, float& inOutMax) {
		constexpr float kEpsilon = 0.0001f;
		if (std::abs(denominator) < kEpsilon) {
			return numerator >= 0.0f;
		}

		const float t = numerator / denominator;
		if (denominator < 0.0f) {
			if (t > inOutMax) {
				return false;
			}
			if (t > inOutMin) {
				inOutMin = t;
			}
		} else {
			if (t < inOutMin) {
				return false;
			}
			if (t < inOutMax) {
				inOutMax = t;
			}
		}

		return true;
	};

	return clip(-diffX, startX - minX, tMin, tMax) &&
	       clip(diffX, maxX - startX, tMin, tMax) &&
	       clip(-diffY, startY - minY, tMin, tMax) &&
	       clip(diffY, maxY - startY, tMin, tMax);
}

float ThetaStar::Distance(const SearchNode* a, const SearchNode* b) {
	float dx = float(a->x - b->x);
	float dz = float(a->y - b->y);
	return std::sqrt(dx * dx + dz * dz);
}

} // namespace KujakuEngine
