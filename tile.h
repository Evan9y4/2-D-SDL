#ifndef _TILE__H_
#define _TILE__H_

#include "player.h"
#include "plant.h"

class Tile {
	private:
		float x;
		float y;
		float w = 20;
		float h = 20;
	public:
		virtual void update_tile(Player& player);
		float getX() { return x };
		float getY() { return y };
		float getW() { return w };
		float getH() { return h };
};

class PlanterBox : public Tile {
	private:
		bool hasPlant;
		Plant& plant;
	public:
		override void update_tile(Player& player);
		PlanterBox(float x, float y);
};

enum BuyType { seed1, seed2, seed3, seed4, seed5, seed6, upgrade1, upgrade2 };

class MarketTile : public Tile {
	private:
		BuyType type;
	public:
		override void update_tile(Player& player);
		MarketTile(Buytype type, float x, float y);
};
