#include "market.hpp"
#include "player.hpp"
#include "sound.hpp"
#include "engine.hpp"
#include "text_renderer.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string>

Market::Market() {}

void Market::init() {
    columns[0].title = "SUPPLIES";
    columns[0].items = {
        { MarketItemType::WATER_REFILL, "Water Refill",  "Fill watering can +100",    0.f, PlantType::NONE,  100, false, 0 },
        { MarketItemType::SUN_REFILL,   "Light Lamp",    "Add light to tool +100",    5.f, PlantType::NONE,   100, false, 1 },
        { MarketItemType::PESTICIDE,    "Pesticide x3",  "Kills pests",              15.f, PlantType::NONE,    3, false, 2 },
    };

    columns[1].title = "SEEDS";
    columns[1].items = {
        { MarketItemType::PLANT_SEED, "Zinnia x1",    "Fast, $15 reward",        PlantFactory::getSpec(PlantType::ZINNIA).seedCost,    PlantType::ZINNIA,    1, false, 0 },
        { MarketItemType::PLANT_SEED, "Mint x1",      "Very fast, $10",          PlantFactory::getSpec(PlantType::MINT).seedCost,      PlantType::MINT,      1, false, 0 },
        { MarketItemType::PLANT_SEED, "Rose x1",      "Medium, $40 reward",      PlantFactory::getSpec(PlantType::ROSE).seedCost,      PlantType::ROSE,      1, false, 2 },
        { MarketItemType::PLANT_SEED, "Cactus x1",    "Low water, $60",          PlantFactory::getSpec(PlantType::CACTUS).seedCost,    PlantType::CACTUS,    1, false, 3 },
        { MarketItemType::PLANT_SEED, "Fern x1",      "Loves water, $35",        PlantFactory::getSpec(PlantType::FERN).seedCost,      PlantType::FERN,      1, false, 4 },
        { MarketItemType::PLANT_SEED, "Sunflower x1", "Hard, $100 reward",       PlantFactory::getSpec(PlantType::SUNFLOWER).seedCost, PlantType::SUNFLOWER, 1, false, 6 },
    };

    columns[2].title = "UPGRADES";
    columns[2].items = {
        { MarketItemType::UNLOCK_SLOT,  "Unlock Slot",    "Open new garden slot",   50.f, PlantType::NONE, 1, false, 2 },
        { MarketItemType::TOOL_UPGRADE, "Bigger Tank",    "+50 water capacity",     40.f, PlantType::NONE, 1, false, 1 },
        { MarketItemType::TOOL_UPGRADE, "Bigger Lamp",    "+50 light capacity",     45.f, PlantType::NONE, 1, false, 1 },
        { MarketItemType::TOOL_UPGRADE, "Faster Growth",  "Plants grow 20% faster", 80.f, PlantType::NONE, 1, false, 3 },
        { MarketItemType::TOOL_UPGRADE, "Better Harvest", "+20% harvest money",    100.f, PlantType::NONE, 1, false, 5 },
    };

    currentCol   = 0;
    currentRow   = 0;
    currentQuota = 0;
    confirmTimer = 0.f;
    confirmCol   = -1;
    confirmRow   = -1;
}

void Market::unlockForQuota(int quotaLevel) {
    currentQuota = quotaLevel;
}

// returns not locked
static bool isRowSelectable(const std::array<MarketColumn, 3>& cols, int col, int row, int quota) {
    if (col < 0 || col > 2) return false;
    if (row < 0 || row >= (int)cols[col].items.size()) return false;
    return cols[col].items[row].lockedUntilQuota <= quota;
}

void Market::snapToUnlocked(int preferDir) {
    // find nearest unlocked row
    const auto& col = columns[currentCol];
    int sz = (int)col.items.size();
    if (sz == 0) return;

    // try preffered first then switch
    for (int pass = 0; pass < 2; ++pass) {
        int dir = (pass == 0) ? preferDir : -preferDir;
        for (int delta = 0; delta < sz; ++delta) {
            int r = currentRow + delta * dir;
            if (r < 0 || r >= sz) break;
            if (col.items[r].lockedUntilQuota <= currentQuota) {
                currentRow = r;
                return;
            }
        }
    }
}

void Market::moveSelection(int dx, int dy) {
    if (dx != 0) {
        currentCol = std::max(0, std::min(2, currentCol + dx));
        currentRow = std::min(currentRow, (int)columns[currentCol].items.size() - 1);
        snapToUnlocked(1);
    }
    if (dy != 0) {
        int sz = (int)columns[currentCol].items.size();
        int newRow = currentRow + dy;
        while (newRow >= 0 && newRow < sz && columns[currentCol].items[newRow].lockedUntilQuota > currentQuota)
            newRow += dy;
        if (newRow >= 0 && newRow < sz)
            currentRow = newRow;
    }
    Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f);
}

void Market::confirmPurchase(Player& player) {
    if (currentCol < 0 || currentCol > 2) return;
    auto& col = columns[currentCol];
    if (currentRow < 0 || currentRow >= (int)col.items.size()) return;
    MarketItem& item = col.items[currentRow];
    if (item.lockedUntilQuota > currentQuota) return;
    buyItem(item, player);
}

void Market::buyItem(MarketItem& item, Player& player) {
    if (!player.spendMoney(item.cost)) return;
    Sound::instance().play(SoundID::BUY_ITEM, 0.3f);

    switch (item.type) {
        case MarketItemType::PLANT_SEED:
            player.addSeed(item.seedType, item.quantity);
            player.selectSeed(item.seedType);
            player.selectTool(Tool::PLANT);
            break;
        case MarketItemType::WATER_REFILL:
            player.refillWater((float)item.quantity);
            break;
        case MarketItemType::SUN_REFILL:
            player.refillSun((float)item.quantity);
            player.setEverHadSun();
            break;
        case MarketItemType::PESTICIDE:
            player.refillPesticide((float)item.quantity);
            player.setEverHadPesticide();
            break;
        case MarketItemType::UNLOCK_SLOT:
            // handled in gamescene onPurchase()
            break;
        case MarketItemType::TOOL_UPGRADE:
            if (item.name == "Bigger Tank") {
                player.upgradeWaterMax(50.f);
            } 
            else if (item.name == "Bigger Lamp") {
                player.upgradeSunMax(50.f);
            }
            // other upgrades in gamescene
            // add more if need
            break;
        case MarketItemType::FERTILIZER:
            break;
    }

    // Trigger confirmation flash on the purchased item
    confirmCol   = currentCol;
    confirmRow   = currentRow;
    confirmTimer = 1.4f;   // seconds to show the flash

    if (onPurchase) onPurchase(item, player);
}

void Market::update(float dt) {
    if (confirmTimer > 0.f) confirmTimer -= dt;

    if (!open) return;
    for (const auto& evt : Engine::keyEvents) {
        if (evt.type != SDL_EVENT_KEY_DOWN) continue;
        SDL_Keycode key = evt.key.key;
        if (key == SDLK_UP    || key == SDLK_W) moveSelection(0, -1);
        if (key == SDLK_DOWN  || key == SDLK_S) moveSelection(0,  1);
        if (key == SDLK_LEFT  || key == SDLK_A) moveSelection(-1,  0);
        if (key == SDLK_RIGHT || key == SDLK_D) moveSelection( 1,  0);
        if (key == SDLK_ESCAPE) closeMarket();
    }
}

void Market::render(SDL_Renderer* renderer, int screenW, int screenH, const Player& player) {
    if (!open) return;

    auto& txt = TextRenderer::instance();

    // dark overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    SDL_FRect overlay = { 0.f, 0.f, (float)screenW, (float)screenH };
    SDL_RenderFillRect(renderer, &overlay);

    float panelW = (float)screenW * 0.88f;
    float panelH = (float)screenH * 0.80f;
    float panelX = ((float)screenW - panelW) / 2.f;
    float panelY = ((float)screenH - panelH) / 2.f;

    SDL_SetRenderDrawColor(renderer, 22, 40, 14, 255);
    SDL_FRect panel = { panelX, panelY, panelW, panelH };
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 120, 190, 70, 255);
    SDL_RenderRect(renderer, &panel);

    txt.drawCentered(renderer, "BLOOM BLOOM MARKET", panelX, panelY + 6.f, panelW, 26, {160,240,110,255});
    txt.drawCentered(renderer, "A/D:column   W/S:item   Space:buy   Tab:close",
                     panelX, panelY + panelH - 24.f, panelW, 17, {100,160,70,255});

    float colW    = (panelW - 16.f) / 3.f;
    float colTopY = panelY + 36.f;
    float colH    = panelH - 64.f;

    for (int c = 0; c < 3; ++c) {
        float cx     = panelX + 8.f + c * colW;
        bool  colSel = (c == currentCol);

        SDL_SetRenderDrawColor(renderer, colSel?28:16, colSel?52:32, colSel?16:9, 255);
        SDL_FRect colRect = { cx, colTopY, colW - 4.f, colH };
        SDL_RenderFillRect(renderer, &colRect);
        SDL_SetRenderDrawColor(renderer, colSel?130:55, colSel?200:90, colSel?75:36, 255);
        SDL_RenderRect(renderer, &colRect);

        SDL_Color titleCol = colSel ? SDL_Color{200,255,150,255} : SDL_Color{120,180,80,255};
        txt.drawCentered(renderer, columns[c].title, cx, colTopY + 5.f, colW - 4.f, 17, titleCol);

        SDL_SetRenderDrawColor(renderer, colSel?110:55, colSel?180:90, colSel?65:36, 255);
        SDL_RenderLine(renderer, (int)(cx+4), (int)(colTopY+29), (int)(cx+colW-8), (int)(colTopY+29));

        float itemH = 70.f;
        float iy    = colTopY + 34.f;

        for (int r = 0; r < (int)columns[c].items.size(); ++r) {
            const MarketItem& item = columns[c].items[r];
            bool rowSel = colSel && (r == currentRow);
            bool locked = (item.lockedUntilQuota > currentQuota);

            SDL_FRect itemRect = { cx + 4.f, iy, colW - 12.f, itemH - 3.f };

            // backgrounds
            if (locked)
                SDL_SetRenderDrawColor(renderer, 18, 22, 13, 255);
            else if (rowSel)
                SDL_SetRenderDrawColor(renderer, 50, 95, 30, 255);
            else
                SDL_SetRenderDrawColor(renderer, 28, 50, 16, 255);
            SDL_RenderFillRect(renderer, &itemRect);

            SDL_Color borderC = locked  ? SDL_Color{48,55,38,255}
                              : rowSel  ? SDL_Color{190,250,110,255}
                                        : SDL_Color{75,125,46,255};
            SDL_SetRenderDrawColor(renderer, borderC.r, borderC.g, borderC.b, borderC.a);
            SDL_RenderRect(renderer, &itemRect);

            // right-side boundary
            float rightColX = cx + itemRect.w - 56.f;

            if (locked) {
                // name and description
                txt.draw(renderer, item.name,        cx + 8.f, iy + 6.f,  13, {70,80,58,255});
                txt.draw(renderer, item.description, cx + 8.f, iy + 23.f, 14, {55,63,44,255});
                // lock level
                txt.draw(renderer, "Lv." + std::to_string(item.lockedUntilQuota),
                         rightColX + 6.f, iy + 14.f, 16, {95,80,55,255});
            } 
            else {
                // name description
                SDL_Color nc = rowSel ? SDL_Color{225,255,185,255} : SDL_Color{175,220,135,255};
                txt.draw(renderer, item.name,        cx + 8.f, iy + 4.f,  14, nc);
                txt.draw(renderer, item.description, cx + 8.f, iy + 21.f, 14, {125,170,95,255});

                // divider
                SDL_SetRenderDrawColor(renderer, 70, 110, 45, 150);
                SDL_RenderLine(renderer, (int)rightColX, (int)(iy + 4), (int)rightColX, (int)(iy + itemH - 6));

                // price
                txt.draw(renderer, "$" + std::to_string((int)item.cost),
                         rightColX + 6.f, iy + 4.f, 18, {255,210,65,255});

                // info under price
                std::string contextStr = "";
                SDL_Color contextCol   = {160, 200, 140, 255};

                if (item.type == MarketItemType::PLANT_SEED) {
                    int have = player.getSeedCount(item.seedType);
                    contextStr = "x" + std::to_string(have);
                    contextCol = {140, 200, 160, 255};
                } 
                else if (item.type == MarketItemType::WATER_REFILL) {
                    contextStr = std::to_string((int)player.getWaterLevel())
                                 + "/" + std::to_string((int)player.getWaterMax());
                    contextCol = {100, 160, 220, 255};
                } 
                else if (item.type == MarketItemType::SUN_REFILL) {
                    contextStr = std::to_string((int)player.getSunLevel())
                                 + "/" + std::to_string((int)player.getSunMax());
                    contextCol = {220, 200, 60, 255};
                } 
                else if (item.type == MarketItemType::PESTICIDE) {
                    contextStr = "x" + std::to_string((int)player.getPesticideLevel());
                    contextCol = {140, 210, 80, 255};
                }

                if (!contextStr.empty()) {
                    txt.draw(renderer, contextStr, rightColX + 6.f, iy + 22.f, 11, contextCol);
                }

                // Confirmation flash: "BOUGHT!" replaces context info briefly
                bool isConfirm = (confirmTimer > 0.f && confirmCol == c && confirmRow == r);
                if (isConfirm) {
                    float alpha = std::min(1.f, confirmTimer / 0.4f);
                    Uint8 a = (Uint8)(alpha * 255.f);
                    txt.draw(renderer, "GOT!", rightColX + 4.f, iy + 35.f, 16, {120,255,120,a});
                }
            }

            iy += itemH;
        }
    }
}

void Market::moveSelection(int dy) { moveSelection(0, dy); }
