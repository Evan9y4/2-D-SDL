#include "pest.hpp"
#include "plant.hpp"
#include <SDL3/SDL.h>
#include <string>

Pest::Pest(const PestSpec& s, int targetGridIndex, SDL_Renderer* renderer)
    : spec(s), targetCell(targetGridIndex) {
    tickTimer  = s.damageInterval; 
    pesticideHp = s.pesticideHp;
    alive       = true;

    const char* basePath = SDL_GetBasePath();
    std::string fullPath = basePath ? (std::string(basePath) + spec.spritePath) : spec.spritePath;

    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (surface) {
        frameW  = surface->w / 4;  // sprite sheet 4 frames, use first
        frameH  = surface->h;
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!texture) {
            SDL_Log("SDL_CreateTextureFromSurface failed for pest %s: %s",
                    fullPath.c_str(), SDL_GetError());
        }
    } else {
        SDL_Log("Could not load pest sprite: %s — %s", fullPath.c_str(), SDL_GetError());
    }
}

Pest::~Pest() {
    if (texture) SDL_DestroyTexture(texture);
}

bool Pest::update(float dt, float& outDamage) {
    outDamage = 0.f;
    if (!alive) return false;

    tickTimer -= dt;
    if (tickTimer <= 0.f) {
        tickTimer = spec.damageInterval;
        outDamage = spec.damagePerTick;
    }
    return true;
}

void Pest::render(SDL_Renderer* renderer, SDL_FRect destRect) {
    if (!texture) {
        // error red square
        SDL_SetRenderDrawColor(renderer, 200, 50, 50, 200);
        SDL_RenderFillRect(renderer, &destRect);
        return;
    }
    SDL_FRect src = { 0.f, 0.f, (float)frameW, (float)frameH };
    SDL_RenderTexture(renderer, texture, &src, &destRect);
}

void Pest::applyPesticide(float strength) {
    pesticideHp -= strength;
    if (pesticideHp <= 0.f) alive = false;
}

bool Pest::affects(PestType pest, int plantTypeInt) {
    PlantType pt = (PlantType)plantTypeInt;
    switch (pest) {
        case PestType::APHID:
            // easy — targets mint and zinnia
            return pt == PlantType::MINT || pt == PlantType::ZINNIA;
        case PestType::SPIDER_MITE:
            // medium — targets fern and rose
            return pt == PlantType::FERN || pt == PlantType::ROSE;
        case PestType::MEALYBUG:
            // hard — targets cactus and sunflower
            return pt == PlantType::CACTUS || pt == PlantType::SUNFLOWER;
        default:
            return false;
    }
}
