// Grid - a class to store planter box and market tiles in a grid
#ifndef _GRID__H_
#define _GRID__H_

#include "tile.h"
#include <vector>

class Grid {
	private:
		// vector of Tiles
		std::vector<Tile> grid;
		// # of rows and columns, will be useful for player cursor movement on the grid
		int rows;
		int cols;
	public:
		void update_grid();
		Tile& getTile(int index);
		Grid(std::vector<Tile> tiles, int rows, int cols);
};
