#include "tile.h"
#include "player.h"

PlanterBox::PlanterBox(float x, float y) {
	this->x = x;
	this->y = y;
	this-hasPlant = false;
}


void PlanterBox::update_tile(Player& player) {
	if (hasPlant) {
	//todo define behavior for plants with tools
	}
	else {
	//todo define behavior for planting a plant
	}
}
