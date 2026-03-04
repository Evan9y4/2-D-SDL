#ifndef __HPP_PEST__
#define __HPP_PEST__

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

enum class PestType {
    APHID     = 0,
    SPIDER_MITE = 1,
    MEALYBUG  = 2 
};

struct PestSpec {
    PestType    type;
    std::string name;
    float       spawnRate;
    float       damagePerTick;
    float       damageInterval;
    std::string spritePath;
    float       pesticideHp = 1.f;
};

class Pest {
public:
    Pest() = default;
    Pest(const PestSpec& spec, int targetGridIndex, SDL_Renderer* renderer);
    ~Pest();

    bool update(float dt, float& outDamage);
    void render(SDL_Renderer* renderer, SDL_FRect destRect);

    void applyPesticide(float strength);
    bool isAlive() const { return alive; }

    int  getTargetCell() const { return targetCell; }
    PestType getType()   const { return spec.type; }

    static bool affects(PestType pest, int plantTypeInt);

private:
    PestSpec     spec{};
    int          targetCell    = -1;
    float        tickTimer     = 0.f;
    float        pesticideHp   = 3.f;
    bool         alive         = true;
    SDL_Texture* texture       = nullptr;
    int          frameW        = 16;
    int          frameH        = 16;
};

#endif // __HPP_PEST__
