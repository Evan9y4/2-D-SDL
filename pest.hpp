#include "quota.hpp"
#include "grid.hpp"
#include "sound.hpp"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>

Quota::Quota() {}

// placeholder stuff
void Quota::buildDefaultQuota(int number, QuotaData& out) {
    // levels now, quota as new gamemode later
    static const QuotaData presets[] = {
        /* 1*/ {1,   60.f,  3, 0.f, 9999.f, 0, 1.0f, "Get started! Earn $60."},
        /* 2*/ {2,  120.f,  4, 0.f, 9999.f, 0, 1.1f, "Keep growing!"},
        /* 3*/ {3,  200.f,  5, 0.f, 9999.f, 0, 1.2f, "First pests appear soon."},
        /* 4*/ {4,  300.f,  6, 0.f, 9999.f, 0, 1.4f, "Pests are here!"},
        /* 5*/ {5,  450.f,  8, 0.f, 9999.f, 0, 1.5f, "Mid-game. Expand your garden."},
        /* 6*/ {6,  600.f,  8, 0.f, 9999.f, 0, 1.6f, "High value plants needed."},
        /* 7*/ {7,  800.f, 10, 0.f, 9999.f, 0, 1.8f, "Getting tough!"},
        /* 8*/ {8, 1000.f, 10, 0.f, 9999.f, 0, 2.0f, "Master gardener."},
        /* 9*/ {9, 1300.f, 12, 0.f, 9999.f, 0, 2.2f, "Almost there!"},
        /*10*/{10, 1700.f, 14, 0.f, 9999.f, 0, 2.5f, "Final quota. Good luck!"}
    };
    if (number >= 1 && number <= 10) {
        out = presets[number - 1];
    }
}

void Quota::buildRandomQuota(int number, QuotaData& out) {
    // endless mode - randomize with scaling difficulty
    float scale   = 1.f + (number - 10) * 0.15f;
    out.quotaNumber    = number;
    out.moneyGoal      = 1000.f * scale;
    out.plantGoal      = 14 + (number - 10);
    out.timeLimit      = std::max(60.f, 120.f - (number - 10) * 2.f);
    out.pestSpawnInterval = std::max(4.f, 8.f - (number - 10) * 0.3f);
    out.maxPests       = std::min(8, 5 + (number - 10) / 3);
    out.difficulty     = scale;
    out.description    = "Endless mode - survive!";
}

void Quota::loadQuota(int number) {
    data = {};
    harvestCounts.clear();
    if (number <= 10) {
        buildDefaultQuota(number, data);
    } else {
        buildRandomQuota(number, data);
    }
    // pests don't appear until quota 4
    if (data.quotaNumber < 4) {
        data.maxPests = 0;
        data.pestSpawnInterval = 9999.f;
    }
}

void Quota::startQuota() {
    active      = true;
    timedOut    = false;
    elapsed     = 0.f;
    moneyEarned = 0.f;
    pestTimer   = data.pestSpawnInterval;
}

void Quota::endQuota(bool success) {
    active = false;
    if (onQuotaEnd) onQuotaEnd(success);
    Sound::instance().play(SoundID::QUOTA_COMPLETE);
}

void Quota::update(float dt, Grid& grid, SDL_Renderer* renderer) {
    if (!active) return;
    elapsed += dt;
    // no time limit now — checked by addMoney() isComplete()
    (void)grid; (void)renderer;
}

void Quota::spawnPestRandomly(Grid& grid, SDL_Renderer* renderer) {
    // random occupied cell
    int total = grid.getTotalCells();
    std::vector<int> candidates;
    for (int i = 0; i < total; ++i) {
        Cell* c = grid.getCell(i);
        if (c && c->state == CellState::PLANTED && c->plant && !c->plant->isDead())
            candidates.push_back(i);
    }
    if (candidates.empty()) return;

    int cellIdx = candidates[rand() % candidates.size()];
    // pick pest
    Cell* cell = grid.getCell(cellIdx);
    if (!cell || !cell->plant) return;

    int plantTypeInt = (int)cell->plant->getType();
    std::vector<PestType> validPests;
    if (Pest::affects(PestType::APHID,      plantTypeInt)) validPests.push_back(PestType::APHID);
    if (Pest::affects(PestType::SPIDER_MITE, plantTypeInt)) validPests.push_back(PestType::SPIDER_MITE);
    if (Pest::affects(PestType::MEALYBUG,   plantTypeInt)) validPests.push_back(PestType::MEALYBUG);

    if (validPests.empty()) return;
    PestType chosen = validPests[rand() % validPests.size()];
    grid.spawnPest(chosen, cellIdx, renderer);
}

bool Quota::isComplete() const {
    if (moneyEarned < data.moneyGoal) return false;
    // check plant requirements
    for (const auto& [type, count] : data.plantRequirements) {
        int done = 0;
        for (const auto& [ht, hc] : harvestCounts) {
            if (ht == type) { done = hc; break; }
        }
        if (done < count) return false;
    }
    return true;
}

void Quota::addHarvest(PlantType type) {
    for (auto& [t, c] : harvestCounts) {
        if (t == type) { c++; return; }
    }
    harvestCounts.push_back({ type, 1 });
}

void Quota::renderHUD(SDL_Renderer* /*renderer*/, int /*screenW*/) {
    // timer removed
}
