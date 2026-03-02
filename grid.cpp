#include <vector>
#include "grid.h"
#include "tile.h"

Grid::Grid(std::vector<Tile> tiles, int rows, int cols) {
	this->grid = tiles;
	this->rows = rows;
	this->cols = cols;
}

void Grid::update_grid() {
	for (const auto& tile : grid) {
		tile.update();
	}
}

Tile& Grid::getTile(int index) {
	return this->grid[index];
}	
