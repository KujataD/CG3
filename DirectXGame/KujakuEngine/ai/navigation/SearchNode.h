#pragma once
#include <cmath>
namespace KujakuEngine{

struct SearchNode {
	int32_t x, y;
	float g; // スタート地点から調査マスまでの距離（実コスト）
	float h; // 調査マスからゴールまでの距離（推定コスト）
	float f; // g + h（優先度）
	SearchNode* parent;

	SearchNode(int32_t x, int32_t y) : x(x), y(y), g(0.0f), h(0.0f), f(0.0f), parent(nullptr) {}
	bool operator>(const SearchNode& other) const { return f > other.f; }
};

}