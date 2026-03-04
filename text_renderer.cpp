#include "game_scene.hpp"
#include "engine.hpp"
#include "sound.hpp"
#include "text_renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <cmath>

GameScene::GameScene() : grid(4, 4) {}
GameScene::~GameScene() { shutdown(); }

// icon helpers

SDL_Texture* GameScene::loadIcon(SDL_Renderer* renderer, const char* path) {
    const char* base = SDL_GetBasePath();
    std::string full = base ? (std::string(base) + path) : path;
    SDL_Surface* s = IMG_Load(full.c_str());
    if (!s) { SDL_Log("Icon load failed: %s — %s", full.c_str(), SDL_GetError()); return nullptr; }
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_DestroySurface(s);
    return t;
}

void GameScene::renderIcon(SDL_Renderer* renderer, SDL_Texture* tex, float x, float y, float size) {
    if (!tex) return;
    SDL_FRect dst = { x, y, size, size };
    SDL_RenderTexture(renderer, tex, nullptr, &dst);
}

bool GameScene::init(SDL_Renderer* renderer) {
    rendererRef = renderer;
    player.setRenderer(renderer);
    TextRenderer::instance().init("assets/fonts/font.ttf");
    PlantFactory::initDefaults();
    grid.init(renderer, 4);
    market.init();

    // load icons
    iconWaterDrop = loadIcon(renderer, "assets/sprites/icon_water_drop.png");
    iconH2O       = loadIcon(renderer, "assets/sprites/icon_h2o.png");
    iconSun       = loadIcon(renderer, "assets/sprites/icon_sun.png");
    iconPesticide = loadIcon(renderer, "assets/sprites/icon_pesticide.png");
    iconMarket    = loadIcon(renderer, "assets/sprites/icon_market.png");

    market.onPurchase = [this](MarketItem& item, Player& /*p*/) {
        if (item.type == MarketItemType::UNLOCK_SLOT) {
            for (int i = 0; i < grid.getTotalCells(); ++i) {
                if (!grid.isCellUnlocked(i)) { grid.unlockCell(i); break; }
            }
        }
    };

    grid.onHarvest = [this](int /*cell*/, float money) {
        player.addMoney(money);
        quota.addMoney(money);
        score += money;
        if (quota.isComplete()) quota.endQuota(true);
    };

    grid.onPlantDied = [](int /*cell*/) {};

    quota.onQuotaEnd = [this](bool success) {
        showLevelResults(success);
    };

    Sound& snd = Sound::instance();
    snd.init();
    snd.loadSound(SoundID::PEST_SPAWN,    "assets/audio/pest_spawn.wav");
    snd.loadSound(SoundID::PEST_DAMAGE,   "assets/audio/pest_damage.wav");
    snd.loadSound(SoundID::CURSOR_MOVE,    "assets/audio/cursor_move.wav");
    snd.loadSound(SoundID::PLAYER_INPUT,  "assets/audio/click.wav");
    snd.loadSound(SoundID::BUY_ITEM,      "assets/audio/buy.wav");
    snd.loadSound(SoundID::WATER_PLANT,   "assets/audio/water.wav");
    snd.loadSound(SoundID::HARVEST,       "assets/audio/harvest.wav");
    snd.loadSound(SoundID::QUOTA_COMPLETE,"assets/audio/quota_complete.wav");
    if (musicEnabled) {
        snd.loadMusic("assets/audio/bgm.wav");
        snd.setMusicVolume(0.75f);
        snd.playMusic(true);
    }
    return true;
}

// confetti

void GameScene::spawnConfetti(int screenW, int screenH) {
    static const SDL_Color palette[] = {
        {255, 80, 80, 255}, {255, 200, 40, 255}, {80, 220, 80, 255},
        {80, 180, 255, 255}, {220, 80, 255, 255}, {255, 140, 40, 255},
        {80, 255, 200, 255}, {255, 255, 80, 255}
    };
    for (int i = 0; i < 120; ++i) {
        ConfettiParticle p;
        p.x    = (float)(rand() % screenW);
        p.y    = (float)(-(rand() % 80));          // start above screen
        p.vx   = ((rand() % 100) - 50) * 0.04f;   // -2 to +2
        p.vy   = 60.f + (rand() % 80);             // fall speed
        p.size = 4.f + (rand() % 5);
        p.color = palette[rand() % 8];
        p.maxLife = 2.5f + (rand() % 100) * 0.01f;
        p.life = p.maxLife;
        confetti.push_back(p);
    }
}

void GameScene::updateConfetti(float dt) {
    for (auto& p : confetti) {
        p.x   += p.vx * dt * 60.f;
        p.y   += p.vy * dt;
        p.vx  += ((rand() % 10) - 5) * 0.002f;  // slight drift
        p.life -= dt;
    }
    confetti.erase(std::remove_if(confetti.begin(), confetti.end(),
        [](const ConfettiParticle& p){ return p.life <= 0.f; }), confetti.end());
}

void GameScene::renderConfetti(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto& p : confetti) {
        float alpha = std::min(1.f, p.life / 0.5f);  // fade out last 0.5s
        SDL_SetRenderDrawColor(renderer, p.color.r, p.color.g, p.color.b, (Uint8)(alpha * 255));
        SDL_FRect r = { p.x, p.y, p.size, p.size };
        SDL_RenderFillRect(renderer, &r);
    }
}

void GameScene::update(float dt) {
    updateConfetti(dt);

    for (const auto& evt : Engine::keyEvents) {
        if (evt.type != SDL_EVENT_KEY_DOWN) continue;
        SDL_Keycode key = evt.key.key;

        if (state == GameState::TITLE) {
            if (key == SDLK_RETURN || key == SDLK_SPACE) startGame(rendererRef);
            if (key == SDLK_C) { creditsReturnState = GameState::TITLE; state = GameState::CREDITS; }
            continue;
        }
        if (state == GameState::CREDITS) {
            if (key == SDLK_ESCAPE || key == SDLK_RETURN || key == SDLK_SPACE) state = creditsReturnState;
            continue;
        }
        if (state == GameState::LEVEL_RESULTS) {
            if (key == SDLK_SPACE || key == SDLK_RETURN) {
                if (lastLevelSuccess) nextLevel(rendererRef);
                else                  returnToTitle();
            }
            continue;
        }
        if (state == GameState::PAUSED) {
            int sel = (int)pauseSelected;
            int cnt = (int)PauseMenuOption::COUNT;
            if (key == SDLK_UP   || key == SDLK_W) { sel = (sel - 1 + cnt) % cnt; pauseSelected = (PauseMenuOption)sel; Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f); }
            if (key == SDLK_DOWN || key == SDLK_S) { sel = (sel + 1) % cnt;        pauseSelected = (PauseMenuOption)sel; Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f); }
            if (key == SDLK_RETURN || key == SDLK_SPACE) {
                if (showControlsOverlay) { showControlsOverlay = false; continue; }
                switch (pauseSelected) {
                    case PauseMenuOption::RESUME:       state = GameState::PLAYING; break;
                    case PauseMenuOption::CREDITS:      creditsReturnState = GameState::PAUSED; state = GameState::CREDITS; break;
                    case PauseMenuOption::CONTROLS:     showControlsOverlay = !showControlsOverlay; break;
                    case PauseMenuOption::TOGGLE_SOUND: {
                        soundEnabled = !soundEnabled;
                        // soundEnabled flag controls SFX volume in play() calls
                        // mute/unmute BGM and all sound
                        if (!soundEnabled) Sound::instance().setMusicVolume(0.f);
                        else if (musicEnabled) Sound::instance().setMusicVolume(0.75f);
                        break;
                    }
                    case PauseMenuOption::TOGGLE_BGM: {
                        musicEnabled = !musicEnabled;
                        if (musicEnabled && soundEnabled) {
                            Sound::instance().playMusic(true);
                            Sound::instance().setMusicVolume(0.75f);
                        } 
                        else {
                            Sound::instance().stopMusic();
                        }
                        break;
                    }
                    case PauseMenuOption::TITLE:        returnToTitle(); break;
                    case PauseMenuOption::EXIT:         SDL_Quit(); exit(0); break;
                    default: break;
                }
            }
            if (key == SDLK_ESCAPE || key == SDLK_P) {
                if (showControlsOverlay) {
                    showControlsOverlay = false;
                } 
                else {
                    state = GameState::PLAYING;
                }
            }
            continue;
        }
        if (state == GameState::PLAYING || state == GameState::MARKET_OPEN) {
            if (key == SDLK_P && state == GameState::PLAYING) {
                pauseSelected = PauseMenuOption::RESUME;
                showControlsOverlay = false;
                state = GameState::PAUSED;
                continue;
            }
            if (key == SDLK_C && state == GameState::PLAYING) {
                creditsReturnState = GameState::PLAYING;
                state = GameState::CREDITS;
                continue;
            }
            if (key == SDLK_TAB) {
                market.toggle();
                state = market.isOpen() ? GameState::MARKET_OPEN : GameState::PLAYING;
            }
            if (key == SDLK_ESCAPE && state == GameState::MARKET_OPEN) {
                market.closeMarket();
                state = GameState::PLAYING;
            }
        }
    }

    if (state == GameState::PLAYING) {
        player.handleInput(grid, market);
        grid.update(dt);

        pestTimer += dt;
        if (pestTimer >= pestSpawnInterval) {
            pestTimer = 0.f;
            spawnRandomPest();
        }
    }

    if (state == GameState::MARKET_OPEN) {
        market.update(dt);
        for (const auto& evt : Engine::keyEvents) {
            if (evt.type != SDL_EVENT_KEY_DOWN) continue;
            if (evt.key.key == SDLK_SPACE || evt.key.key == SDLK_RETURN)
                market.confirmPurchase(player);
        }
    }
}

void GameScene::render(SDL_Renderer* renderer) {
    int screenW = Engine::instance().getScreenWidth();
    int screenH = Engine::instance().getScreenHeight();

    switch (state) {
        case GameState::TITLE:         renderTitle(renderer);        return;
        case GameState::LEVEL_RESULTS: renderLevelResults(renderer); return;
        case GameState::CREDITS:       renderCredits(renderer);      return;
        case GameState::PAUSED:        break;  // render game then overlay
        default: break;
    }

    float hudH = 90.f, gridY = 0.f, sidebarW = 220.f;
    float gridW = (float)screenW - sidebarW;
    float gridH = (float)screenH - hudH - gridY;

    grid.render(renderer, { 0.f, gridY, gridW, gridH });
    renderSidebar(renderer, gridW, gridY, sidebarW, gridH);
    player.renderHUD(renderer, screenW, screenH);

    // warning overlays
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < grid.getTotalCells(); ++i) {
        Cell* cell = grid.getCell(i);
        if (!cell || cell->state != CellState::PLANTED || !cell->plant) continue;
        Plant* p = cell->plant.get();

        int col = i % 4, row = i / 4;
        float cw = gridW / 4.f, ch = gridH / 4.f;
        float rx = col * cw, ry = gridY + row * ch;

        // all icon size
        float iconSize = std::min(cw * 0.37f, 33.f);
        float iconX    = rx + 3.f;
        float step     = iconSize + 2.f;
        int   slot     = 0; 

        auto drawSlot = [&](int s, SDL_Texture* tex,
                             const std::string& label, SDL_Color labelCol) {
            float iy = ry + ch - iconSize - 3.f - s * step;
            if (iy < ry) return;  // would go above cell top
            SDL_FRect badge = { iconX - 1.f, iy - 1.f, iconSize + 2.f, iconSize + 2.f };
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
            SDL_RenderFillRect(renderer, &badge);
            if (tex) {
                SDL_FRect dst = { iconX, iy, iconSize, iconSize };
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
            } else {
                TextRenderer::instance().drawCentered(renderer, label,
                    iconX, iy, iconSize, (int)(iconSize * 0.85f), labelCol);
            }
        };

        // pests first, then harvest, then needs-water
        for (int pi = 0; pi < (int)cell->pests.size(); ++pi) {
            float iy = ry + ch - iconSize - 3.f - slot * step;
            if (iy >= ry) {
                SDL_FRect badge = { iconX - 1.f, iy - 1.f, iconSize + 2.f, iconSize + 2.f };
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
                SDL_RenderFillRect(renderer, &badge);
                SDL_FRect dst = { iconX, iy, iconSize, iconSize };
                cell->pests[pi]->render(renderer, dst);
            }
            slot++;
        }

        // harvest ready
        if (p->isReadyToHarvest()) {
            float decayT    = p->getHarvestTimer();
            float remaining = 10.f - decayT;
            int secsLeft    = std::max(1, (int)remaining + 1);
            float danger    = std::min(1.f, decayT / 10.f);
            Uint8 rr = (Uint8)(180 + danger * 75.f);
            Uint8 gg = (Uint8)std::max(0.f, 100.f - danger * 100.f);
            Uint8 bb = (Uint8)std::max(0.f, 80.f - danger * 80.f);
            drawSlot(slot, nullptr, std::to_string(secsLeft), {rr, gg, bb, 255});
            slot++;
        }

        // water
        if (!p->hasBeenWatered() && !p->isReadyToHarvest()) {
            drawSlot(slot, iconWaterDrop, "", {255,255,255,255});
            slot++;
        }
    }
    market.render(renderer, screenW, screenH, player);
    renderConfetti(renderer);
    if (state == GameState::PAUSED) renderPauseMenu(renderer);
}

void GameScene::shutdown() {
    if (iconWaterDrop) { SDL_DestroyTexture(iconWaterDrop); iconWaterDrop = nullptr; }
    if (iconH2O)       { SDL_DestroyTexture(iconH2O);       iconH2O       = nullptr; }
    if (iconSun)       { SDL_DestroyTexture(iconSun);        iconSun       = nullptr; }
    if (iconPesticide) { SDL_DestroyTexture(iconPesticide);  iconPesticide = nullptr; }
    if (iconMarket)    { SDL_DestroyTexture(iconMarket);     iconMarket    = nullptr; }
    TextRenderer::instance().shutdown();
    Sound::instance().shutdown();
}

// state transitions


void GameScene::startGame(SDL_Renderer* renderer) {
    score      = 0.f;
    quotaLevel = 1;
    pestTimer  = 0.f;
    confetti.clear();
    grid.init(renderer, 4);
    market.init();
    updatePestDifficulty();
    quota.loadQuota(quotaLevel);
    quota.startQuota();
    state = GameState::PLAYING;
}

void GameScene::nextLevel(SDL_Renderer* renderer) {
    quotaLevel++;
    pestTimer = 0.f;
    updatePestDifficulty();
    market.unlockForQuota(quotaLevel);
    quota.loadQuota(quotaLevel);
    quota.startQuota();
    state = GameState::PLAYING;
}

void GameScene::showLevelResults(bool success) {
    lastLevelSuccess = success;
    state = GameState::LEVEL_RESULTS;
    if (success) {
        int sw = Engine::instance().getScreenWidth();
        int sh = Engine::instance().getScreenHeight();
        spawnConfetti(sw, sh);
    } 
    else {
        bestScore = std::max(bestScore, score);
    }
}

void GameScene::returnToTitle() {
    bestScore = std::max(bestScore, score);
    score = 0.f;
    confetti.clear();
    state = GameState::TITLE;
}

void GameScene::updatePestDifficulty() {
    pestSpawnInterval = std::max(8.f, 30.f - (quotaLevel - 1) * 2.f);
}

void GameScene::spawnRandomPest() {
    std::vector<int> candidates;
    for (int i = 0; i < grid.getTotalCells(); ++i) {
        Cell* c = grid.getCell(i);
        if (c && c->state == CellState::PLANTED && c->plant && !c->plant->isDead())
            candidates.push_back(i);
    }
    if (candidates.empty()) return;

    int cellIdx = candidates[rand() % candidates.size()];
    Cell* cell  = grid.getCell(cellIdx);
    if (!cell || !cell->plant) return;

    int plantTypeInt = (int)cell->plant->getType();
    std::vector<PestType> valid;
    if (Pest::affects(PestType::APHID,       plantTypeInt)) valid.push_back(PestType::APHID);
    if (Pest::affects(PestType::SPIDER_MITE, plantTypeInt)) valid.push_back(PestType::SPIDER_MITE);
    if (Pest::affects(PestType::MEALYBUG,    plantTypeInt)) valid.push_back(PestType::MEALYBUG);
    if (valid.empty()) return;

    grid.spawnPest(valid[rand() % valid.size()], cellIdx, rendererRef);
}

// draw help

void GameScene::drawFilledRect(SDL_Renderer* r, float x, float y, float w, float h,
                                Uint8 rr, Uint8 gg, Uint8 bb, Uint8 aa) {
    SDL_SetRenderDrawColor(r, rr, gg, bb, aa);
    SDL_FRect rect = { x, y, w, h };
    SDL_RenderFillRect(r, &rect);
}

void GameScene::drawBorderedRect(SDL_Renderer* r, float x, float y, float w, float h,
                                  SDL_Color fill, SDL_Color border) {
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
    SDL_FRect rect = { x, y, w, h };
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
    SDL_RenderRect(r, &rect);
}

static void drawPlantBar(SDL_Renderer* r, float x, float y, float w, float h,
                          float valuePct, float optimalPct, bool overloaded,
                          SDL_Color fillColor) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 25, 25, 25, 255);
    SDL_FRect bg = { x, y, w, h };
    SDL_RenderFillRect(r, &bg);

    float fillFrac = std::max(0.f, std::min(1.f, valuePct / 100.f));
    SDL_Color fc   = overloaded ? SDL_Color{200, 35, 35, 255} : fillColor;
    SDL_SetRenderDrawColor(r, fc.r, fc.g, fc.b, fc.a);
    if (fillFrac > 0.f) {
        SDL_FRect fill = { x, y, fillFrac * w, h };
        SDL_RenderFillRect(r, &fill);
    }

    float ozStart = std::max(0.f, optimalPct - 8.f) / 100.f;
    float ozEnd   = std::min(100.f, optimalPct + 8.f) / 100.f;
    SDL_SetRenderDrawColor(r, 255, 240, 60, 80);
    SDL_FRect optZone = { x + ozStart * w, y, (ozEnd - ozStart) * w, h };
    SDL_RenderFillRect(r, &optZone);

    float markerX = x + (optimalPct / 100.f) * w;
    SDL_SetRenderDrawColor(r, 255, 240, 80, 220);
    SDL_RenderLine(r, (int)markerX, (int)y, (int)markerX, (int)(y + h));

    SDL_SetRenderDrawColor(r, 100, 130, 100, 255);
    SDL_RenderRect(r, &bg);
}

// screen renderers

void GameScene::renderTitle(SDL_Renderer* renderer) {
    auto& txt = TextRenderer::instance();
    float W = (float)Engine::instance().getScreenWidth();
    float H = (float)Engine::instance().getScreenHeight();
    drawFilledRect(renderer, 0, 0, W, H, 18, 45, 18);
    float cx=W*0.2f, cy=H*0.18f, cw=W*0.6f, ch=H*0.32f;
    drawBorderedRect(renderer, cx, cy, cw, ch, {35,70,35,255}, {100,210,80,255});
    txt.drawCentered(renderer, "BLOOM BLOOM",    cx, cy+ch*0.10f, cw, 56, {160,240,120,255});
    txt.drawCentered(renderer, "A Plant Tycoon", cx, cy+ch*0.65f, cw, 26, {120,190,90,255});
    txt.drawCentered(renderer, "PRESS SPACE TO PLAY",          0, H*0.60f, W, 32, {200,255,160,255});
    txt.drawCentered(renderer, "C  -  Credits",                0, H*0.68f, W, 24, {140,190,110,255});
    if (bestScore > 0.f)
        txt.drawCentered(renderer, "Best: $"+std::to_string((int)bestScore), 0, H*0.80f, W, 22, {100,160,80,255});
    txt.drawCentered(renderer, "WASD/Arrows:Move  Space:Action  Q/E:Tool Z/X:Swap Seed  Tab:Market",
                     0, H*0.92f, W, 19, {80,120,60,255});
}

void GameScene::renderSidebar(SDL_Renderer* renderer, float x, float y, float w, float h) {
    auto& txt = TextRenderer::instance();
    drawFilledRect(renderer, x, y, w, h, 22, 32, 14);
    SDL_SetRenderDrawColor(renderer, 70, 110, 40, 255);
    SDL_RenderLine(renderer, (int)x, (int)y, (int)x, (int)(y+h));

    float tx = x + 12.f, ty = y + 12.f;

    // quota progress
    txt.draw(renderer, "Level "+std::to_string(quotaLevel), tx, ty, 19, {120,200,80,255}); ty += 27.f;
    float moneyPct = std::min(1.f, quota.getMoneyProgress() / std::max(1.f, quota.getMoneyGoal()));
    float barW = w - 24.f;
    SDL_SetRenderDrawColor(renderer, 40, 60, 30, 255);
    SDL_FRect barBg = { tx, ty, barW, 12.f };
    SDL_RenderFillRect(renderer, &barBg);
    SDL_FRect barFill = { tx, ty, barW * moneyPct, 12.f };
    SDL_SetRenderDrawColor(renderer, 80, 200, 80, 255);
    SDL_RenderFillRect(renderer, &barFill);
    SDL_SetRenderDrawColor(renderer, 100, 160, 70, 255);
    SDL_RenderRect(renderer, &barBg);
    ty += 21.f;
    txt.draw(renderer, "$"+std::to_string((int)quota.getMoneyProgress())
             +" / $"+std::to_string((int)quota.getMoneyGoal()),
             tx, ty, 15, {160,220,120,255}); ty += 23.f;

    SDL_SetRenderDrawColor(renderer, 70, 110, 40, 180);
    SDL_RenderLine(renderer, (int)tx, (int)ty, (int)(x+w-12), (int)ty); ty += 13.f;

    // current cell info
    int idx = grid.getCursorIndex();
    Cell* cell = grid.getCell(idx);
    txt.draw(renderer, "Cell "+std::to_string(idx+1), tx, ty, 17, {160,210,120,255}); ty += 25.f;

    if (cell && cell->state == CellState::PLANTED && cell->plant) {
        Plant* p = cell->plant.get();
        txt.draw(renderer, p->getName(), tx, ty, 21, {200,240,150,255}); ty += 27.f;

        txt.draw(renderer, "HP", tx, ty, 15, {160,160,160,255});
        txt.draw(renderer, std::to_string((int)p->getHealth()), tx+28.f, ty, 15, {220,100,80,255}); ty += 21.f;

        float bw = w - 38.f;

        // water bar
        bool wOver = p->isWaterOverloaded();
        txt.draw(renderer, "H2O", tx, ty, 14, wOver ? SDL_Color{220,60,60,255} : SDL_Color{80,160,220,255});
        ty += 17.f;
        drawPlantBar(renderer, tx, ty, bw, 10.f,
                     p->getWaterPct(), p->getOptimalWaterPct(), wOver, {70, 140, 220, 255});
        txt.draw(renderer, std::to_string((int)p->getWaterPct()) + "%",
                 tx + bw + 3.f, ty, 13,
                 wOver ? SDL_Color{220,80,80,255} : SDL_Color{140,195,230,255});
        ty += 19.f;

        // sun bar
        bool sOver = p->isSunOverloaded();
        txt.draw(renderer, "SUN", tx, ty, 14, sOver ? SDL_Color{220,60,60,255} : SDL_Color{230,200,60,255});
        ty += 17.f;
        drawPlantBar(renderer, tx, ty, bw, 10.f,
                     p->getSunPct(), p->getOptimalSunPct(), sOver, {215, 190, 45, 255});
        txt.draw(renderer, std::to_string((int)p->getSunPct()) + "%",
                 tx + bw + 3.f, ty, 13,
                 sOver ? SDL_Color{220,80,80,255} : SDL_Color{200,185,100,255});
        ty += 19.f;

        txt.draw(renderer, "Grow", tx, ty, 14, {160,160,160,255});
        txt.draw(renderer, std::to_string((int)(p->getGrowthPct()*100.f))+"%", tx+32.f, ty, 14, {160,220,120,255}); ty += 19.f;

        if (!p->hasBeenWatered() && !p->isReadyToHarvest()) {
            txt.draw(renderer, "Needs water!", tx, ty, 15, {80,160,230,255}); ty += 19.f;
        }

        if (p->isReadyToHarvest()) {
            float mult = p->computeHarvestMultiplier();
            SDL_Color mc = mult >= 1.8f ? SDL_Color{100,255,100,255}
                         : mult >= 1.4f ? SDL_Color{220,220,80,255}
                                        : SDL_Color{200,150,80,255};
            txt.draw(renderer, "> HARVEST! <", tx, ty, 17, {255,240,80,255}); ty += 21.f;
            txt.draw(renderer, "Mult: x" + std::to_string(mult).substr(0,4), tx, ty, 11, mc); ty += 19.f;

            float decayT    = p->getHarvestTimer();
            float remaining = 10.f - decayT;
            if (decayT > 0.f) {
                float danger = std::min(1.f, decayT / 10.f);
                Uint8 rr = (Uint8)(180 + danger * 75.f);
                Uint8 gg = (Uint8)std::max(0.f, 80.f - danger * 80.f);
                Uint8 bb = (Uint8)std::max(0.f, 80.f - danger * 80.f);
                SDL_Color timerCol = {rr, gg, bb, 255};

                txt.draw(renderer, "Die in: " + std::to_string((int)remaining + 1) + "s",
                         tx, ty, 12, timerCol); ty += 19.f;

                float decayFrac = std::max(0.f, remaining / 10.f);
                SDL_SetRenderDrawColor(renderer, 35, 18, 18, 255);
                SDL_FRect dbb = { tx, ty, bw, 7.f };
                SDL_RenderFillRect(renderer, &dbb);
                SDL_SetRenderDrawColor(renderer, rr, gg, bb, 255);
                SDL_FRect dbf = { tx, ty, bw * decayFrac, 7.f };
                SDL_RenderFillRect(renderer, &dbf);
                SDL_SetRenderDrawColor(renderer, 100, 60, 60, 255);
                SDL_RenderRect(renderer, &dbb);
                ty += 16.f;
            }
        }

        if (!cell->pests.empty()) {
            txt.draw(renderer, "Pests x" + std::to_string(cell->pests.size()), tx, ty, 16, {240,80,80,255}); ty += 19.f;
            txt.draw(renderer, "Use PESTICIDE tool", tx, ty, 14, {200,60,60,255});
        }

    } else if (cell && cell->state == CellState::EMPTY) {
        txt.draw(renderer, "(empty)",       tx, ty, 17, {80,110,60,255}); ty+=23.f;
        txt.draw(renderer, "Buy seed from", tx, ty, 15, {80,110,60,255}); ty+=19.f;
        txt.draw(renderer, "Market (Tab),", tx, ty, 15, {80,110,60,255}); ty+=19.f;
        txt.draw(renderer, "then use PLANT",tx, ty, 15, {80,110,60,255});
    } else if (cell && cell->state == CellState::LOCKED) {
        txt.draw(renderer, "(locked)",      tx, ty, 17, {80,70,50,255}); ty+=23.f;
        txt.draw(renderer, "Unlock in",     tx, ty, 15, {80,70,50,255}); ty+=19.f;
        txt.draw(renderer, "Market (Tab)",  tx, ty, 15, {80,70,50,255});
    }

    // total earned with controls
    float bottomSectionH = 13.f + 25.f + 33.f + 13.f + 18.f*7.f + 13.f;
    ty = y + h - bottomSectionH;
    SDL_SetRenderDrawColor(renderer, 70, 110, 40, 180);
    SDL_RenderLine(renderer, (int)tx, (int)ty, (int)(x+w-12), (int)ty); ty+=13.f;
    txt.draw(renderer, "TOTAL EARNED", tx, ty, 16, {120,200,80,255}); ty+=25.f;
    txt.draw(renderer, "$"+std::to_string((int)score), tx, ty, 22, {160,240,120,255}); ty+=33.f;

    SDL_SetRenderDrawColor(renderer, 70, 110, 40, 180);
    SDL_RenderLine(renderer, (int)tx, (int)ty, (int)(x+w-12), (int)ty); ty+=13.f;
    txt.draw(renderer, "WASD/Arrows:Move",  tx, ty, 13, {80,130,60,255}); ty+=17.f;
    txt.draw(renderer, "Space:Action",      tx, ty, 13, {80,130,60,255}); ty+=17.f;
    txt.draw(renderer, "Q/E:Tool",          tx, ty, 13, {80,130,60,255}); ty+=17.f;
    txt.draw(renderer, "Z/X:Select Seed",   tx, ty, 13, {80,130,60,255}); ty+=17.f;
    txt.draw(renderer, "Tab:Market",        tx, ty, 13, {80,130,60,255}); ty+=17.f;
    txt.draw(renderer, "C:Credits",         tx, ty, 13, {80,130,60,255}); ty+=17.f;
    txt.draw(renderer, "P:Pause",           tx, ty, 13, {80,130,60,255});
}

void GameScene::renderLevelResults(SDL_Renderer* renderer) {
    auto& txt = TextRenderer::instance();
    float W = (float)Engine::instance().getScreenWidth();
    float H = (float)Engine::instance().getScreenHeight();

    if (lastLevelSuccess) drawFilledRect(renderer, 0,0,W,H, 12,40,12);
    else                  drawFilledRect(renderer, 0,0,W,H, 40,12,12);

    renderConfetti(renderer);

    float cx=W*0.2f, cy=H*0.22f, cw=W*0.6f, ch=H*0.52f;
    SDL_Color fill   = lastLevelSuccess ? SDL_Color{30,70,30,255}   : SDL_Color{80,18,18,255};
    SDL_Color border = lastLevelSuccess ? SDL_Color{100,220,80,255} : SDL_Color{220,80,60,255};
    drawBorderedRect(renderer, cx, cy, cw, ch, fill, border);

    SDL_Color tc = lastLevelSuccess ? SDL_Color{160,255,120,255} : SDL_Color{255,120,100,255};
    txt.drawCentered(renderer, lastLevelSuccess ? "LEVEL UP!" : "LEVEL FAILED",
                     cx, cy+20.f, cw, 44, tc);

    txt.drawCentered(renderer, "Level "+std::to_string(quotaLevel),
                     cx, cy+80.f, cw, 26, {200,220,180,255});
    txt.drawCentered(renderer, "Total earned: $"+std::to_string((int)score),
                     cx, cy+110.f, cw, 24, {200,220,180,255});

    if (lastLevelSuccess) {
        txt.drawCentered(renderer, "Pests are now stronger!",
                         cx, cy+145.f, cw, 20, {220,160,80,255});
        txt.drawCentered(renderer, "New items unlocked in Market!",
                         cx, cy+165.f, cw, 20, {160,220,120,255});
        txt.drawCentered(renderer, "Press SPACE to close",
                         cx, cy+ch-45.f, cw, 24, {160,230,130,255});
    } else {
        txt.drawCentered(renderer, "Press SPACE to return to menu",
                         cx, cy+ch-45.f, cw, 24, {220,140,120,255});
    }
}

void GameScene::renderPauseMenu(SDL_Renderer* renderer) {
    auto& txt = TextRenderer::instance();
    int screenW = Engine::instance().getScreenWidth();
    int screenH = Engine::instance().getScreenHeight();
    float W = (float)screenW, H = (float)screenH;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_FRect overlay = { 0.f, 0.f, W, H };
    SDL_RenderFillRect(renderer, &overlay);

    if (showControlsOverlay) {
        renderControlsOverlay(renderer);
        return;
    }

    float pw = W * 0.42f, ph = H * 0.72f;
    float px = (W - pw) * 0.5f, py = (H - ph) * 0.5f;
    drawBorderedRect(renderer, px, py, pw, ph, {20, 40, 18, 255}, {100, 200, 70, 255});

    txt.drawCentered(renderer, "PAUSED", px, py + 14.f, pw, 38, {180, 255, 130, 255});

    static const char* labels[] = {
        "Resume", "Credits", "Controls", "Toggle Sound", "Toggle BGM", "Title Screen (Resets Progress)", "Exit Game"
    };
    int cnt = (int)PauseMenuOption::COUNT;
    float itemH = 42.f;
    float listY  = py + 72.f;

    for (int i = 0; i < cnt; ++i) {
        bool sel = ((int)pauseSelected == i);
        float iy = listY + i * itemH;
        SDL_FRect itemRect = { px + 16.f, iy, pw - 32.f, itemH - 6.f };
        SDL_SetRenderDrawColor(renderer, sel ? 45 : 25, sel ? 85 : 45, sel ? 25 : 14, 255);
        SDL_RenderFillRect(renderer, &itemRect);
        SDL_SetRenderDrawColor(renderer, sel ? 160 : 60, sel ? 230 : 100, sel ? 90 : 40, 255);
        SDL_RenderRect(renderer, &itemRect);

        std::string label = labels[i];
        if (i == (int)PauseMenuOption::TOGGLE_SOUND)
            label = std::string("Sound: ") + (soundEnabled ? "ON" : "OFF");
        else if (i == (int)PauseMenuOption::TOGGLE_BGM)
            label = std::string("BGM: ") + (musicEnabled ? "ON" : "OFF");

        SDL_Color tc = sel ? SDL_Color{220, 255, 170, 255} : SDL_Color{140, 190, 100, 255};
        txt.drawCentered(renderer, label, px + 16.f, iy + 6.f, pw - 32.f, 22, tc);
    }

    txt.drawCentered(renderer, "W/S or Arrows: Move   Space/Enter: Select   P/Esc: Resume",
                     px, py + ph - 28.f, pw, 14, {80, 130, 60, 255});
}

void GameScene::renderControlsOverlay(SDL_Renderer* renderer) {
    auto& txt = TextRenderer::instance();
    int screenW = Engine::instance().getScreenWidth();
    int screenH = Engine::instance().getScreenHeight();
    float W = (float)screenW, H = (float)screenH;

    float pw = W * 0.60f, ph = H * 0.78f;
    float px = (W - pw) * 0.5f, py = (H - ph) * 0.5f;
    drawBorderedRect(renderer, px, py, pw, ph, {20, 40, 18, 255}, {100, 200, 70, 255});

    txt.drawCentered(renderer, "CONTROLS", px, py + 14.f, pw, 34, {180, 255, 130, 255});

    float ltx = px + 28.f, lty = py + 62.f;
    float lineH = 26.f;
    SDL_Color hdr = {140, 220, 100, 255};
    SDL_Color val = {200, 235, 160, 255};

    auto line = [&](const char* label, const char* binding) {
        txt.draw(renderer, label, ltx, lty, 17, hdr);
        txt.draw(renderer, binding, ltx + pw * 0.44f, lty, 17, val);
        lty += lineH;
    };

    line("Move Cursor",       "WASD / Arrow Keys");
    line("Use Tool",          "Space");
    line("Cycle Tool (fwd)",  "E");
    line("Cycle Tool (back)", "Q");
    line("Next Seed",         "X");
    line("Prev Seed",         "Z");
    line("Open Market",       "Tab");
    lty += 6.f;
    line("Pause",             "P");
    line("Credits",           "C");
    lty += 6.f;
    line("Close Market",      "Esc / Tab");
    line("Back to Title",     "Pause Menu");
    lty += 6.f;
    txt.draw(renderer, "Tools:", ltx, lty, 17, {120, 200, 90, 255}); lty += lineH;
    line("  Water",           "Use with H2O in tank");
    line("  Light",           "Buy Light Lamp first");
    line("  Pesticide",       "Buy Pesticide first");
    line("  Plant",           "Select seed then use");
    line("  Remove",          "Removes current plant");

    txt.drawCentered(renderer, "Press Space or Esc to go back",
                     px, py + ph - 30.f, pw, 17, {80, 130, 60, 255});
}


void GameScene::renderCredits(SDL_Renderer* renderer) {
    auto& txt = TextRenderer::instance();
    float W = (float)Engine::instance().getScreenWidth();
    float H = (float)Engine::instance().getScreenHeight();
    drawFilledRect(renderer, 0, 0, W, H, 14, 28, 14);
    float cx=W*0.2f, cy=H*0.12f, cw=W*0.6f, ch=H*0.76f;
    drawBorderedRect(renderer, cx, cy, cw, ch, {25,45,20,255}, {90,180,70,255});
    float ltx=cx+30.f, lty=cy+20.f;
    txt.drawCentered(renderer, "CREDITS", cx, lty, cw, 40, {160,240,120,255}); lty+=70.f;
    txt.draw(renderer, "Game Design & Programming:", ltx, lty, 20, {180,220,140,255}); lty+=34.f;
    txt.draw(renderer, "  Josh Burgenmeyer, Angel Mejia-Velasquez, Evan Bower",     ltx, lty, 22, {220,255,180,255}); lty+=46.f;
    txt.draw(renderer, "Built with:",         ltx, lty, 20, {180,220,140,255}); lty+=34.f;
    txt.draw(renderer, "  SDL3 + SDL3_image", ltx, lty, 19, {200,230,160,255}); lty+=32.f;
    txt.draw(renderer, "  SDL3_ttf",          ltx, lty, 19, {200,230,160,255}); lty+=32.f;
    txt.draw(renderer, "  miniaudio",         ltx, lty, 19, {200,230,160,255}); lty+=46.f;
    txt.draw(renderer, "Assets:",             ltx, lty, 20, {180,220,140,255}); lty+=34.f;
    txt.draw(renderer, "  mixkit sound effects", ltx, lty, 18, {160,200,120,255});
    txt.drawCentered(renderer, "Press ESC or SPACE to return", cx, cy+ch-40.f, cw, 22, {120,180,90,255});
}
