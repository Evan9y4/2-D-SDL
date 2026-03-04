#include "player.hpp"
#include "grid.hpp"
#include "market.hpp"
#include "sound.hpp"
#include "engine.hpp"
#include "text_renderer.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string>
#include <vector>

Player::Player() {
    seedInventory[(int)PlantType::ZINNIA] = 2;
}

bool Player::spendMoney(float amt) {
    if (money < amt) return false;
    money -= amt;
    return true;
}

int Player::getSeedCount(PlantType t) const {
    auto it = seedInventory.find((int)t);
    return it != seedInventory.end() ? it->second : 0;
}

void Player::addSeed(PlantType t, int count) {
    seedInventory[(int)t] += count;
}

bool Player::useSeed(PlantType t) {
    auto it = seedInventory.find((int)t);
    if (it == seedInventory.end() || it->second <= 0) return false;
    it->second--;
    return true;
}

std::vector<Tool> Player::getAvailableTools() const {
    std::vector<Tool> tools = { Tool::WATER };
    if (everHadSun)       tools.push_back(Tool::SUN);
    if (everHadPesticide) tools.push_back(Tool::PESTICIDE);
    tools.push_back(Tool::PLANT);
    tools.push_back(Tool::REMOVE);
    return tools;
}

void Player::nextTool() {
    auto tools = getAvailableTools();
    auto it = std::find(tools.begin(), tools.end(), selectedTool);
    if (it == tools.end()) { selectedTool = tools[0]; return; }
    ++it;
    if (it == tools.end()) it = tools.begin();
    selectedTool = *it;
    Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f);
}

void Player::prevTool() {
    auto tools = getAvailableTools();
    auto it = std::find(tools.begin(), tools.end(), selectedTool);
    if (it == tools.end()) { selectedTool = tools[0]; return; }
    if (it == tools.begin()) it = tools.end();
    --it;
    selectedTool = *it;
    Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f);
}

void Player::nextSeed() {
    std::vector<PlantType> owned;
    for (auto& [k, v] : seedInventory)
        if (v > 0) owned.push_back((PlantType)k);
    if (owned.empty()) return;
    std::sort(owned.begin(), owned.end());
    auto it = std::find(owned.begin(), owned.end(), selectedSeed);
    if (it == owned.end()) { selectedSeed = owned[0]; return; }
    ++it;
    if (it == owned.end()) it = owned.begin();
    selectedSeed = *it;
    Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f);
}

void Player::prevSeed() {
    std::vector<PlantType> owned;
    for (auto& [k, v] : seedInventory)
        if (v > 0) owned.push_back((PlantType)k);
    if (owned.empty()) return;
    std::sort(owned.begin(), owned.end());
    auto it = std::find(owned.begin(), owned.end(), selectedSeed);
    if (it == owned.end()) { selectedSeed = owned[0]; return; }
    if (it == owned.begin()) it = owned.end();
    --it;
    selectedSeed = *it;
    Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f);
}

void Player::handleInput(Grid& grid, Market& /*market*/) {
    for (const auto& evt : Engine::keyEvents) {
        if (evt.type != SDL_EVENT_KEY_DOWN) continue;
        SDL_Keycode key = evt.key.key;

        if (key == SDLK_UP    || key == SDLK_W) grid.moveCursor(0, -1);
        if (key == SDLK_DOWN  || key == SDLK_S) grid.moveCursor(0,  1);
        if (key == SDLK_LEFT  || key == SDLK_A) grid.moveCursor(-1, 0);
        if (key == SDLK_RIGHT || key == SDLK_D) grid.moveCursor( 1, 0);

        if (key == SDLK_Q) prevTool();
        if (key == SDLK_E) nextTool();

        if (key == SDLK_Z) prevSeed();
        if (key == SDLK_X) nextSeed();

        if (key == SDLK_SPACE) {
            int idx = grid.getCursorIndex();
            Cell* cell = grid.getCell(idx);

            if (cell && cell->state == CellState::PLANTED && cell->plant
                && cell->plant->isReadyToHarvest()) {
                grid.harvestPlant(idx);
                continue;
            }

            switch (selectedTool) {
                case Tool::WATER:
                    if (waterLevel >= 10.f) {
                        grid.waterPlant(idx, 25.f);
                        waterLevel -= 10.f;
                    }
                    break;
                case Tool::SUN:
                    if (sunLevel >= 10.f) {
                        grid.applyFertilizer(idx, 25.f);
                        sunLevel -= 10.f;
                    }
                    break;
                case Tool::PESTICIDE:
                    if (pesticideLevel >= 1.f) {
                        grid.applyPesticide(idx, 1.f);
                        pesticideLevel -= 1.f;
                    }
                    break;
                case Tool::PLANT:
                    if (getSeedCount(selectedSeed) > 0) {
                        if (grid.placePlant(idx, selectedSeed, renderer))
                            useSeed(selectedSeed);
                    }
                    break;
                case Tool::REMOVE:
                    grid.removePlant(idx);
                    break;
            }
            Sound::instance().play(SoundID::PLAYER_INPUT, 0.35f);
        }
    }
}

void Player::drawSimpleBar(SDL_Renderer* r, float x, float y, float w, float h,
                            float value, float maxVal, SDL_Color color) {
    // background
    SDL_SetRenderDrawColor(r, 30, 30, 30, 255);
    SDL_FRect bg = { x, y, w, h };
    SDL_RenderFillRect(r, &bg);

    // fill bar
    float frac = (maxVal > 0.f) ? std::max(0.f, std::min(1.f, value / maxVal)) : 0.f;
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_FRect fill = { x, y, frac * w, h };
    SDL_RenderFillRect(r, &fill);

    // border
    SDL_SetRenderDrawColor(r, 120, 120, 120, 200);
    SDL_RenderRect(r, &bg);
}

void Player::renderHUD(SDL_Renderer* renderer, int screenW, int screenH) {
    auto& txt  = TextRenderer::instance();
    float hudH = 90.f;
    float hudY = (float)screenH - hudH;

    SDL_SetRenderDrawColor(renderer, 18, 26, 12, 240);
    SDL_FRect hudBg = { 0.f, hudY, (float)screenW, hudH };
    SDL_RenderFillRect(renderer, &hudBg);
    SDL_SetRenderDrawColor(renderer, 70, 110, 40, 255);
    SDL_RenderRect(renderer, &hudBg);

    // resources
    float bx = 14.f, by = hudY + 8.f;
    float barW = 100.f, barH2 = 12.f;
    float labelW = 36.f;

    // water
    txt.draw(renderer, "H2O", bx, by, 17, {100,170,240,255});
    drawSimpleBar(renderer, bx + labelW, by + 1.f, barW, barH2, waterLevel, waterMax, {60,140,220,255});
    txt.draw(renderer, std::to_string((int)waterLevel) + "/" + std::to_string((int)waterMax),
             bx + labelW + barW + 5.f, by, 14, {140,190,230,255});
    by += 24.f;

    if (everHadSun) {
        txt.draw(renderer, "LIT", bx, by, 17, {240,220,80,255});
        drawSimpleBar(renderer, bx + labelW, by + 1.f, barW, barH2, sunLevel, sunMax, {220,190,50,255});
        txt.draw(renderer, std::to_string((int)sunLevel) + "/" + std::to_string((int)sunMax),
                 bx + labelW + barW + 5.f, by, 14, {200,185,100,255});
        by += 24.f;
    }

    if (everHadPesticide) {
        txt.draw(renderer, "PST", bx, by, 17, {160,220,80,255});
        drawSimpleBar(renderer, bx + labelW, by + 1.f, barW, barH2, pesticideLevel, 10.f, {130,200,70,255});
        txt.draw(renderer, std::to_string((int)pesticideLevel),
                 bx + labelW + barW + 5.f, by, 14, {155,205,115,255});
    }

    // money and seeds
    float mx = 210.f;
    txt.draw(renderer, "$" + std::to_string((int)money), mx, hudY + 6.f, 36, {130,230,90,255});

    float sx = mx, sy = hudY + 52.f;
    txt.draw(renderer, "Seeds:", sx, sy, 16, {160,200,120,255}); sx += 54.f;
    static const struct { PlantType t; const char* abbr; } seedDisplay[] = {
        {PlantType::ZINNIA,"Zinnia"},{PlantType::MINT,"Mint"},{PlantType::ROSE,"Rose"},
        {PlantType::CACTUS,"Cactus"},{PlantType::FERN,"Fern"},{PlantType::SUNFLOWER,"Sunflower"}
    };
    for (auto& sd : seedDisplay) {
        int cnt = getSeedCount(sd.t);
        if (cnt <= 0) continue;
        SDL_Color col = (sd.t == selectedSeed)
                        ? SDL_Color{255,240,80,255} : SDL_Color{180,220,140,255};
        txt.draw(renderer, std::string(sd.abbr) + ":" + std::to_string(cnt), sx, sy, 12, col);
        sx += 54.f;
    }
    txt.draw(renderer, "Seeds inside inventory", mx, hudY + 70.f, 14, {80,120,55,255});

    // tool selector
    auto tools = getAvailableTools();
    int numTools = (int)tools.size();

    float slotW   = 62.f;
    float slotH   = 26.f;
    float slotGap = 3.f;
    float totalW  = numTools * slotW + (numTools - 1) * slotGap;

    float toolAreaRight = (float)screenW - 6.f;
    float toolStartX    = toolAreaRight - totalW;
    float toolY         = hudY + 4.f;  // top of HUD

    for (int i = 0; i < numTools; ++i) {
        Tool t = tools[i];
        const char* name = "";
        switch(t) {
            case Tool::WATER:     name = "WATER"; break;
            case Tool::SUN:       name = "LIGHT"; break;
            case Tool::PESTICIDE: name = "PEST";  break;
            case Tool::PLANT:     name = "PLANT"; break;
            case Tool::REMOVE:    name = "REMOVE"; break;
        }
        bool sel = (t == selectedTool);
        float tx = toolStartX + i * (slotW + slotGap);
        SDL_FRect slot = { tx, toolY, slotW, slotH };
        SDL_SetRenderDrawColor(renderer, sel?55:22, sel?95:38, sel?28:12, 255);
        SDL_RenderFillRect(renderer, &slot);
        SDL_SetRenderDrawColor(renderer, sel?200:75, sel?235:110, sel?100:45, 255);
        SDL_RenderRect(renderer, &slot);
        SDL_Color tc = sel ? SDL_Color{255,240,80,255} : SDL_Color{140,180,100,255};
        txt.drawCentered(renderer, name, tx, toolY + (slotH - 14.f) * 0.5f, slotW, 13, tc);
    }

    // tool desc
    float descY = toolY + slotH + 4.f;
    if (selectedTool == Tool::PLANT) {
        static const char* fullNames[] = {"","Zinnia","Mint","Rose","Cactus","Fern","Sunflower"};
        int si = (int)selectedSeed;
        txt.draw(renderer, "Plant: ", toolStartX, descY, 15, {160,200,120,255});
        txt.draw(renderer, si >= 1 && si <= 6 ? fullNames[si] : "?",
                 toolStartX + 44.f, descY, 15, {220,255,160,255});
        txt.draw(renderer, "(" + std::to_string(getSeedCount(selectedSeed)) + " left)",
                 toolStartX + 44.f, descY + 14.f, 14, {160,200,120,255});
    }

    txt.draw(renderer, "Q/E: seelect tool    Z/X: swap seed", toolStartX, hudY + hudH - 14.f, 14, {80,120,55,255});
}
