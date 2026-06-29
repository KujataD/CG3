#include "AStar.h"
#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_set>

namespace KujakuEngine {

// aはbより後回しにすべきかどうか
// 例) a>bの時、小さいbを優先すべきなので、後回しにすべき
struct CompareFCost {
	bool operator()(SearchNode* a, SearchNode* b) const { return a->f > b->f; }
};

void AStar::Init(const Grid* grid) {
	assert(grid);
	grid_ = grid;
}

std::vector<GridIndex> AStar::FindPath(GridIndex start, GridIndex goal) {
	// 返り値
	std::vector<GridIndex> path;

	// 次に調べる候補ノード。優先度付きキューを用いて、自動でコストが低い順になる。
	std::priority_queue<SearchNode*, std::vector<SearchNode*>, CompareFCost> openList;
	// オープンリストに入っている場所のリスト
	std::vector<std::vector<bool>> inOpen(grid_->height, std::vector<bool>(grid_->width, false));

	// 記録してきたすべてのノード
	std::vector<SearchNode*> allNodes;

	// すでに調べた場所
	std::vector<std::vector<bool>> closed(grid_->height, std::vector<bool>(grid_->width, false));

	// --- スタートノード ---
	SearchNode* startNode = new SearchNode(start.x, start.y);
	startNode->g = 0.0f;
	startNode->h = NavigationUtil::Heuristic(start, goal);
	startNode->f = startNode->h;
	startNode->parent = nullptr;

	openList.push(startNode); // push() で追加
	allNodes.push_back(startNode);

	// 調べていないノードがなくなるまでループ
	while (!openList.empty()) {

		SearchNode* currentNode = openList.top(); // 先頭取得
		openList.pop();                           // 先頭削除

		// --- ゴール判定 ---
		if (currentNode->x == static_cast<int32_t>(goal.x) && currentNode->y == static_cast<int32_t>(goal.y)) {

			// 親をたどってpath配列に追加していく
			SearchNode* node = currentNode;
			while (node) {
				path.push_back({(uint32_t)node->x, (uint32_t)node->y});
				node = node->parent;
			}

			// ゴール→スタートの順なので順序反転する
			std::reverse(path.begin(), path.end());

			// メモリ解放
			for (SearchNode* n : allNodes) {
				delete n;
			}

			return path;
		}

		closed[currentNode->y][currentNode->x] = true;

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
			uint32_t neighborX = currentNode->x + dx[i];
			uint32_t neighborZ = currentNode->y + dz[i];

			// 範囲チェック
			if (neighborX < 0 || neighborX >= grid_->width) {
				continue;
			}
			if (neighborZ < 0 || neighborZ >= grid_->height) {
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
				bool xWalkable = IsWalkable(currentNode->x + dx[i], currentNode->y);
				// 右上に進む場合、上が壁でないかどうか
				bool zWalkable = IsWalkable(currentNode->x, currentNode->y + dz[i]);
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

			neighbor->g = currentNode->g + stepCost;
			neighbor->h = NavigationUtil::Heuristic({(uint32_t)neighborX, (uint32_t)neighborZ}, goal);
			neighbor->f = neighbor->g + neighbor->h;
			neighbor->parent = currentNode;

			openList.push(neighbor); // push() で追加
			allNodes.push_back(neighbor);
		}
	}

	// 見つからなかった場合
	for (SearchNode* n : allNodes) {
		delete n;
	}

	return {};
}
} // namespace KujakuEngine