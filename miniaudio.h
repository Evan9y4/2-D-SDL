#ifndef __HPP_GAME_SCENE__
#define __HPP_GAME_SCENE__

#include "scene.hpp"
#include "grid.hpp"
#include "player.hpp"
#include "market.hpp"
#include "quota.hpp"
#include "pest.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <vector>

enum class GameState {
    TITLE,
    PLAYING,
    MARKET_OPEN,
    LEVEL_RESULTS,
    CREDITS,
    PAUSED
};

enum class PauseMenuOption {
    RESUME = 0,
    CREDITS,
    CONTROLS,
    TOGGLE_SOUND,
    TOGGLE_BGM,
    TITLE,
    EXIT,
    COUNT
};

struct ConfettiParticle {
    float x, y;
    float vx, vy;
    float size;
    SDL_Color color;
    float life;
    float maxLife;
};

class GameScene : public Scene {
public:
    GameScene();
    ~GameScene() override;

    bool init(SDL_Renderer* renderer) override;
    void update(float dt) override;
    void render(SDL_Renderer* renderer) override;
    void shutdown() override;

private:
    Grid   grid;
    Player player;
    Market market;
    Quota  quota;

    GameState state      = GameState::TITLE;
    int       quotaLevel = 1;
    float     score      = 0.f;
    float     bestScore  = 0.f;
    bool      musicEnabled    = true;
    bool      soundEnabled    = true;

    PauseMenuOption pauseSelected = PauseMenuOption::RESUME;
    bool      showControlsOverlay = false;
    GameState creditsReturnState  = GameState::TITLE;
    bool      lastLevelSuccess = false;

    float pestTimer         = 0.f;
    float pestSpawnInterval = 30.f;

    SDL_Renderer* rendererRef = nullptr;

    std::vector<ConfettiParticle> confetti;
    void spawnConfetti(int screenW, int screenH);
    void updateConfetti(float dt);
    void renderConfetti(SDL_Renderer* renderer);

    SDL_Texture* iconWaterDrop = nullptr;
    SDL_Texture* iconH2O       = nullptr;
    SDL_Texture* iconSun       = nullptr;
    SDL_Texture* iconPesticide = nullptr;
    SDL_Texture* iconMarket    = nullptr;

    SDL_Texture* loadIcon(SDL_Renderer* renderer, const char* path);
    void renderIcon(SDL_Renderer* renderer, SDL_Texture* tex, float x, float y, float size);

    void startGame(SDL_Renderer* renderer);
    void nextLevel(SDL_Renderer* renderer);
    void showLevelResults(bool success);
    void returnToTitle();
    void spawnRandomPest();
    void updatePestDifficulty();

    void renderTitle(SDL_Renderer* renderer);
    void renderPauseMenu(SDL_Renderer* renderer);
    void renderControlsOverlay(SDL_Renderer* renderer);
    void renderSidebar(SDL_Renderer* renderer, float x, float y, float w, float h);
    void renderLevelResults(SDL_Renderer* renderer);
    void renderCredits(SDL_Renderer* renderer);

    void drawFilledRect(SDL_Renderer* r, float x, float y, float w, float h,
                        Uint8 rr, Uint8 gg, Uint8 bb, Uint8 aa = 255);
    void drawBorderedRect(SDL_Renderer* r, float x, float y, float w, float h,
                          SDL_Color fill, SDL_Color border);
};

#endif
