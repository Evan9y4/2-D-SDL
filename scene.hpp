#ifndef __HPP_PLANT__
#define __HPP_PLANT__

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

enum class PlantType {
    NONE   = 0,
    ZINNIA = 1,
    MINT   = 2,
    ROSE   = 3,
    CACTUS = 4,
    FERN   = 5,
    SUNFLOWER = 6
};

struct PlantSpec {
    PlantType   type;
    std::string name;

    float optimalWaterPct;
    float optimalSunPct;

    float baseBloomTime;

    float moneyReward;

    float seedCost;

    std::string spritePath;

    int wateringsRequired;
};

enum class GrowthStage {
    SEED    = 0,
    SPROUT  = 1,
    GROWING = 2,
    BLOOMED = 3
};


class Plant {
public:
    Plant() = default;
    Plant(const PlantSpec& spec, SDL_Renderer* renderer);
    ~Plant();

    bool update(float dt);

    void render(SDL_Renderer* renderer, SDL_FRect destRect);

    void water(float pctAmount);
    void applySunlight(float pctAmount);
    void applyFertilizer(float amount); // old

    float harvest();

    void takeDamage(float dmg);
    bool isDead()           const { return health <= 0.0f; }
    bool isReadyToHarvest() const { return growthStage == GrowthStage::BLOOMED; }
    bool hasBeenWatered()   const { return wateringsDone > 0; }

    float getHarvestTimer() const { return harvestDecayTimer; }

    PlantType   getType()           const { return spec.type; }
    std::string getName()           const { return spec.name; }
    float       getHealth()         const { return health; }
    float       getWaterPct()       const { return waterPct; }
    float       getSunPct()         const { return sunPct; }
    float       getOptimalWaterPct()const { return spec.optimalWaterPct; }
    float       getOptimalSunPct()  const { return spec.optimalSunPct; }
    bool        isWaterOverloaded() const { return waterOverloaded; }
    bool        isSunOverloaded()   const { return sunOverloaded; }
    GrowthStage getGrowthStage()    const { return growthStage; }
    float       getGrowthPct()      const;
    float       computeHarvestMultiplier() const;

private:
    float computeGrowthMultiplier() const;

    PlantSpec   spec{};
    float       health       = 100.0f;

    float       waterPct     = 0.0f;
    float       sunPct       = 0.0f;  

    float       frozenWaterPct = 0.0f;
    float       frozenSunPct   = 0.0f;
    bool        valuesFrozen   = false;

    bool        waterOverloaded    = false;
    bool        sunOverloaded      = false;
    float       waterOverloadTimer = 0.f;
    float       sunOverloadTimer   = 0.f;

    static constexpr float kOverloadThreshold    = 120.f;
    static constexpr float kOverloadPenaltyDur   = 8.f;

    float       growthTimer   = 0.0f;
    int         wateringsDone = 0;
    GrowthStage growthStage   = GrowthStage::SEED;

    float       harvestDecayTimer = 0.f;
    static constexpr float kHarvestWindow = 10.f;

    SDL_Texture* texture  = nullptr;
    int          frameW   = 16;
    int          frameH   = 16;
};


class PlantFactory {
public:
    static const PlantSpec& getSpec(PlantType type);
    static void             registerSpec(const PlantSpec& spec);
    static void             initDefaults();
};

#endif // __HPP_PLANT__
