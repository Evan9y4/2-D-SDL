#include "plant.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#include <unordered_map>
#include <stdexcept>

static std::unordered_map<int, PlantSpec> g_specs;
void PlantFactory::initDefaults() {
    // optimalWaterPct and optimalSunPct 0-100
    g_specs[(int)PlantType::ZINNIA] = {
        PlantType::ZINNIA, "Zinnia",
        /*optWater%*/55.f, /*optSun%*/70.f,
        /*bloomTime*/20.f, /*money*/8.f, /*seedCost*/5.f,
        "assets/sprites/zinnia.png", /*waterings*/1
    };
    g_specs[(int)PlantType::MINT] = {
        PlantType::MINT, "Mint",
        /*optWater%*/75.f, /*optSun%*/40.f,
        /*bloomTime*/15.f, /*money*/6.f, /*seedCost*/4.f,
        "assets/sprites/mint.png", /*waterings*/1
    };
    g_specs[(int)PlantType::ROSE] = {
        PlantType::ROSE, "Rose",
        /*optWater%*/60.f, /*optSun%*/80.f,
        /*bloomTime*/25.f, /*money*/30.f, /*seedCost*/20.f,
        "assets/sprites/rose.png", /*waterings*/2
    };
    g_specs[(int)PlantType::CACTUS] = {
        PlantType::CACTUS, "Cactus",
        /*optWater%*/20.f, /*optSun%*/90.f,
        /*bloomTime*/30.f, /*money*/80.f, /*seedCost*/50.f,
        "assets/sprites/cactus.png", /*waterings*/1
    };
    g_specs[(int)PlantType::FERN] = {
        PlantType::FERN, "Fern",
        /*optWater%*/85.f, /*optSun%*/30.f,
        /*bloomTime*/28.f, /*money*/40.f, /*seedCost*/25.f,
        "assets/sprites/fern.png", /*waterings*/2
    };
    g_specs[(int)PlantType::SUNFLOWER] = {
        PlantType::SUNFLOWER, "Sunflower",
        /*optWater%*/50.f, /*optSun%*/95.f,
        /*bloomTime*/45.f, /*money*/150.f, /*seedCost*/100.f,
        "assets/sprites/sunflower.png", /*waterings*/3
    };
}

const PlantSpec& PlantFactory::getSpec(PlantType type) {
    auto it = g_specs.find((int)type);
    if (it == g_specs.end()) throw std::runtime_error("Unknown plant type");
    return it->second;
}

void PlantFactory::registerSpec(const PlantSpec& spec) {
    g_specs[(int)spec.type] = spec;
}


Plant::Plant(const PlantSpec& s, SDL_Renderer* renderer) : spec(s) {
    health        = 100.f;
    waterPct      = 0.f;
    sunPct        = 0.f;
    growthTimer   = 0.f;
    wateringsDone = 0;
    growthStage   = GrowthStage::SEED;

    const char* basePath = SDL_GetBasePath();
    std::string fullPath = basePath ? (std::string(basePath) + spec.spritePath) : spec.spritePath;

    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (surface) {
        frameW  = surface->w / 4;
        frameH  = surface->h;
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!texture)
            SDL_Log("SDL_CreateTextureFromSurface failed for %s: %s", fullPath.c_str(), SDL_GetError());
    } 
    else {
        SDL_Log("Could not load sprite: %s — %s", fullPath.c_str(), SDL_GetError());
    }
}

Plant::~Plant() {
    if (texture) SDL_DestroyTexture(texture);
}

// returns how close to optimal 0-1
static float proximityScore(float pct, float optimal) {
    float diff = std::abs(pct - optimal);
    // within 5% = full score, drops linearly to 0
    float maxDiff = std::max(optimal, 100.f - optimal) + 5.f;
    return std::max(0.f, 1.f - diff / maxDiff);
}

float Plant::computeHarvestMultiplier() const {
    float w = valuesFrozen ? frozenWaterPct : waterPct;
    float s = valuesFrozen ? frozenSunPct   : sunPct;

    float wScore = proximityScore(w, spec.optimalWaterPct);
    float sScore = proximityScore(s, spec.optimalSunPct);
    float combined = (wScore + sScore) * 0.5f; // 0.0-1.0

    return 1.0f + combined;
}

float Plant::computeGrowthMultiplier() const {
    if (!hasBeenWatered()) return 0.f;

    // close to optimal ups grow speed
    float wScore = proximityScore(waterPct, spec.optimalWaterPct);
    float sScore = proximityScore(sunPct,   spec.optimalSunPct);

    // overload halves growth
    float wPenalty = waterOverloaded ? 0.5f : 1.f;
    float sPenalty = sunOverloaded   ? 0.5f : 1.f;

    float mult = (wScore * wPenalty + sScore * sPenalty) * 3.f;
    return std::max(0.25f, std::min(6.f, mult));
}

bool Plant::update(float dt) {
    if (isDead()) return false;

    // harvest decay timer
    if (isReadyToHarvest()) {
        harvestDecayTimer += dt;
        if (harvestDecayTimer >= kHarvestWindow) {
            health = 0.f;
        }
        return false;
    }

    // water and sun going down
    waterPct = std::max(0.f, waterPct - 4.f * dt);
    sunPct   = std::max(0.f, sunPct   - 3.f * dt);

    // overload timer countdown
    if (waterOverloaded) {
        waterOverloadTimer = std::max(0.f, waterOverloadTimer - dt);
        if (waterOverloadTimer <= 0.f) waterOverloaded = false;
    }
    if (sunOverloaded) {
        sunOverloadTimer = std::max(0.f, sunOverloadTimer - dt);
        if (sunOverloadTimer <= 0.f) sunOverloaded = false;
    }

    float mult = computeGrowthMultiplier();
    growthTimer += dt * mult;

    // update growth
    float pct = getGrowthPct();
    if      (pct < 0.25f) growthStage = GrowthStage::SEED;
    else if (pct < 0.60f) growthStage = GrowthStage::SPROUT;
    else if (pct < 1.00f) growthStage = GrowthStage::GROWING;

    bool timerDone    = (growthTimer >= spec.baseBloomTime);
    bool wateringsMet = (wateringsDone >= spec.wateringsRequired);
    if (timerDone && wateringsMet) {
        // freeze values at bloom
        frozenWaterPct = waterPct;
        frozenSunPct   = sunPct;
        valuesFrozen   = true;
        growthStage    = GrowthStage::BLOOMED;
        harvestDecayTimer = 0.f;
        return true;
    }
    return false;
}

void Plant::render(SDL_Renderer* renderer, SDL_FRect destRect) {
    if (!texture) {
        SDL_SetRenderDrawColor(renderer, 80, 160, 80, 255);
        SDL_RenderFillRect(renderer, &destRect);
        return;
    }
    int frame = (int)growthStage;
    SDL_FRect src = { (float)(frame * frameW), 0.f, (float)frameW, (float)frameH };
    SDL_RenderTexture(renderer, texture, &src, &destRect);
}

void Plant::water(float pctAmount) {
    if (isReadyToHarvest()) return; //bloom freezes
    float newVal = waterPct + pctAmount;
    if (newVal > kOverloadThreshold) {
        waterOverloaded    = true;
        waterOverloadTimer = kOverloadPenaltyDur;
    }
    waterPct = std::min(newVal, 150.f); // cap at 150%
    if (growthStage != GrowthStage::BLOOMED) wateringsDone++;
}

void Plant::applySunlight(float pctAmount) {
    if (isReadyToHarvest()) return; // bloom freezes
    float newVal = sunPct + pctAmount;
    if (newVal > kOverloadThreshold) {
        sunOverloaded    = true;
        sunOverloadTimer = kOverloadPenaltyDur;
    }
    sunPct = std::min(newVal, 150.f);
}

void Plant::applyFertilizer(float amount) {
    applySunlight(amount);
}

float Plant::harvest() {
    if (!isReadyToHarvest()) return 0.f;
    float mult   = computeHarvestMultiplier();
    float reward = spec.moneyReward * mult;

    // reset
    health             = 100.f;
    waterPct           = 0.f;
    sunPct             = 0.f;
    frozenWaterPct     = 0.f;
    frozenSunPct       = 0.f;
    valuesFrozen       = false;
    growthTimer        = 0.f;
    wateringsDone      = 0;
    growthStage        = GrowthStage::SEED;
    harvestDecayTimer  = 0.f;
    waterOverloaded    = false;
    sunOverloaded      = false;
    waterOverloadTimer = 0.f;
    sunOverloadTimer   = 0.f;
    return reward;
}

void Plant::takeDamage(float dmg) {
    health = std::max(0.f, health - dmg);
}

float Plant::getGrowthPct() const {
    if (spec.baseBloomTime <= 0.f) return 1.f;
    return std::min(1.f, growthTimer / spec.baseBloomTime);
}
