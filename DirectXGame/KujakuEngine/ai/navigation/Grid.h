#pragma once
#include <cmath>
#include <vector>

// navigationに使うGridの置き場

namespace KujakuEngine {

	// インデックス
	/// <summary>
	/// GridIndex構造体を表します。
	/// </summary>
	struct GridIndex {
		uint32_t x, y;
		bool operator==(const GridIndex& idx) const { return x == idx.x && y == idx.y; }
	};

	// セルの情報
	/// <summary>
	/// GridCell構造体を表します。
	/// </summary>
	struct GridCell {
		bool walkable = true;               // 歩けるかどうか
		float additionalCost = 0.0f; // 追加コスト（毒沼など）
	};

	/// <summary>
	/// グリッドの情報
	/// </summary>
	struct Grid {
		uint32_t width, height;
		std::vector<std::vector<GridCell>> cells;

		/// <summary>
		/// ResizeGridCellsを実行します。
		/// </summary>
		void ResizeGridCells(uint32_t w, uint32_t h) {
			width = w;
			height = h;

			cells.resize(height);
			for (uint32_t i = 0; i < height; ++i) {
				// 1列の要素数を設定(横方向のブロック数)
				cells[i].resize(width);
			}
		}

		/// <summary>
		/// Insideかどうかを返します。
		/// </summary>
		bool IsInside(uint32_t x, uint32_t y) const {
			return (x < width) && (y < height);
		}

		/// <summary>
		/// Walkableかどうかを返します。
		/// </summary>
		bool IsWalkable(uint32_t x, uint32_t y) const {
			if (x < 0 || y < 0) {
				return false;
			}
			return IsInside(static_cast<uint32_t>(x), static_cast<uint32_t>(y)) &&
				cells[y][x].walkable;
		}
	};

} // namespace KujakuEngine