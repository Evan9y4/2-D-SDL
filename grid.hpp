#ifndef __HPP_QUOTA__
#define __HPP_QUOTA__

#include "plant.hpp"
#include "pest.hpp"
#include <vector>
#include <string>
#include <functional>

struct QuotaData {
    int   quotaNumber;
    float moneyGoal;
    int   plantGoal;
    float timeLimit;
    float pestSpawnInterval;
    int   maxPests;
    float difficulty; 
    std::string description;

    std::vector<std::pair<PlantType, int>> plantRequirements;
};

class Quota {
public:
    Quota();

    void loadQuota(int number);
    void startQuota();
    void endQuota(bool success);

    void update(float dt, class Grid& grid, SDL_Renderer* renderer);

    bool isComplete()   const;
    bool isTimedOut()   const { return timedOut; }
    bool isActive()     const { return active; }
    int  getNumber()    const { return data.quotaNumber; }
    float getTimeLeft() const { return std::max(0.f, data.timeLimit - elapsed); }
    float getMoneyProgress() const { return moneyEarned; }
    float getMoneyGoal()     const { return data.moneyGoal; }

    void addMoney(float amt) { moneyEarned += amt; }
    void addHarvest(PlantType type);

    void renderHUD(SDL_Renderer* renderer, int screenW);

    std::function<void(bool success)> onQuotaEnd;

private:
    QuotaData data{};
    bool      active    = false;
    bool      timedOut  = false;
    float     elapsed   = 0.f;
    float     moneyEarned = 0.f;
    float     pestTimer = 0.f;

    std::vector<std::pair<PlantType, int>> harvestCounts;

    void spawnPestRandomly(class Grid& grid, SDL_Renderer* renderer);
    void buildDefaultQuota(int number, QuotaData& out);
    void buildRandomQuota(int number, QuotaData& out);
};

#endif // __HPP_QUOTA__
