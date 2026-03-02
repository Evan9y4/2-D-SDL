#ifndef _PLAYER__H_
#define _PLAYER__H_

#include <vector>
#include "grid.h"

enum Item { Cursor, WaterCan, PortaSun, Seed1, Seed2, Seed3, Seed4, Seed5, Seed6 };

class Player {
	private:
		float x;
		float y;
		float w;
		float h;
		int money;
		Grid plant_grid;
		Grid market_grid;
		Grid current_grid;
		std::vector<int> seeds;
	public:
		static void move();
		static void click();
		static void toggle_menu();
		static void toggle_item();
		static void setMoney(int money);
		static void setSeeds(int index);
};
