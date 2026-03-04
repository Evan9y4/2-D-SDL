#ifndef __HPP_MARKET__
#define __HPP_MARKET__

#include "plant.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <functional>
#include <array>

enum class MarketItemType {
    PLANT_SEED,
    PESTICIDE,
    WATER_REFILL,
    SUN_REFILL,
    FERTILIZER,
    UNLOCK_SLOT,
    TOOL_UPGRADE
};

struct MarketItem {
    MarketItemType type;
    std::string    name;
    std::string    description;
    float          cost;
    PlantType      seedType;
    int            quantity;
    bool           purchased;
    int            lockedUntilQuota;
};

struct MarketColumn {
    std::string             title;
    std::vector<MarketItem> items;
};

class Market {
public:
    Market();
    void init();

    bool isOpen()      const { return open; }
    void openMarket()        { open = true;  }
    void closeMarket()       { open = false; confirmTimer = 0.f; confirmCol = -1; confirmRow = -1; }
    void toggle()            { open = !open; if (!open) { confirmTimer = 0.f; confirmCol = -1; confirmRow = -1; } }

    void moveSelection(int dy);
    void moveSelection(int dx, int dy);
    void confirmPurchase(class Player& player);
    void unlockForQuota(int quotaLevel);

    void update(float dt);
    void render(SDL_Renderer* renderer, int screenW, int screenH, const class Player& player);

    std::function<void(MarketItem&, Player&)> onPurchase;

private:
    bool open         = false;
    int  currentCol   = 0;
    int  currentRow   = 0;
    int  currentQuota = 0;

    float confirmTimer = 0.f;
    int   confirmCol   = -1;
    int   confirmRow   = -1;

    std::array<MarketColumn, 3> columns;

    void buyItem(MarketItem& item, Player& player);

    void snapToUnlocked(int preferDir);
};

#endif
