#include "grid.hpp"
#include "sound.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdlib>

Grid::Grid(int cols, int rows) : cols(cols), rows(rows) {
    cells.resize(cols * rows);
}

bool Grid::init(SDL_Renderer* /*renderer*/, int startUnlocked) {
    for (int i = 0; i < (int)cells.size(); ++i) {
        cells[i].state = (i < startUnlocked) ? CellState::EMPTY : CellState::LOCKED;
    }
    cursorIndex = 0;
    return true;
}

void Grid::update(float dt) {
    for (int i = 0; i < (int)cells.size(); ++i) {
        Cell& cell = cells[i];
        if (cell.state != CellState::PLANTED || !cell.plant) continue;

        // update plant
        bool bloomed = cell.plant->update(dt);
        (void)bloomed; 

        // check if plant dead
        if (cell.plant->isDead()) {
            if (onPlantDied) onPlantDied(i);
            cell.plant.reset();
            cell.state = CellState::EMPTY;
            cell.pests.clear();
            continue;
        }

        // update pests
        checkPests(dt);

        // pest damage to plant
        for (auto& pest : cell.pests) {
            float dmg = 0.f;
            pest->update(dt, dmg);
            if (dmg > 0.f) {
                cell.plant->takeDamage(dmg);
                Sound::instance().play(SoundID::PEST_DAMAGE, 0.25f);
            }
        }

        // kill dead pests
        cell.pests.erase(
            std::remove_if(cell.pests.begin(), cell.pests.end(),
                           [](const std::unique_ptr<Pest>& p){ return !p->isAlive(); }),
            cell.pests.end());
    }
}

void Grid::render(SDL_Renderer* renderer, SDL_FRect bounds) {
    for (int i = 0; i < (int)cells.size(); ++i) {
        SDL_FRect rect = cellRect(i, bounds);
        renderCell(renderer, i, rect);
    }
    // draw cursor highlight
    SDL_FRect cursorRect = cellRect(cursorIndex, bounds);
    renderCursor(renderer, cursorRect);
}

SDL_FRect Grid::cellRect(int index, SDL_FRect bounds) const {
    int col = index % cols;
    int row = index / cols;
    float cw = bounds.w / cols;
    float ch = bounds.h / rows;
    return { bounds.x + col * cw, bounds.y + row * ch, cw, ch };
}

void Grid::renderCell(SDL_Renderer* renderer, int index, SDL_FRect rect) {
    const Cell& cell = cells[index];

    // background 
    if (cell.state == CellState::LOCKED) {
        SDL_SetRenderDrawColor(renderer, 18, 30, 14, 255);   // dark muted green
    } 
    else {
        SDL_SetRenderDrawColor(renderer, 38, 60, 24, 255);   // mid green soil
    }
    SDL_RenderFillRect(renderer, &rect);

    // border
    SDL_SetRenderDrawColor(renderer, 55, 90, 35, 255);
    SDL_RenderRect(renderer, &rect);

    if (cell.state == CellState::PLANTED && cell.plant) {
        // plant sprite
        SDL_FRect plantRect = { rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8 };
        cell.plant->render(renderer, plantRect);

        // warnings handeld by gamescene after render
    } 
    else if (cell.state == CellState::LOCKED) {
        // draw x in cell
        SDL_SetRenderDrawColor(renderer, 35, 55, 22, 255);
        SDL_RenderLine(renderer, (int)(rect.x+6), (int)(rect.y+6),
                                 (int)(rect.x+rect.w-6), (int)(rect.y+rect.h-6));
        SDL_RenderLine(renderer, (int)(rect.x+rect.w-6), (int)(rect.y+6),
                                 (int)(rect.x+6), (int)(rect.y+rect.h-6));
    }
}

void Grid::renderCursor(SDL_Renderer* renderer, SDL_FRect rect) {
    SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
    SDL_FRect outline = { rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2 };
    SDL_RenderRect(renderer, &outline);
    SDL_FRect outline2 = { rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4 };
    SDL_RenderRect(renderer, &outline2);
}

bool Grid::placePlant(int index, PlantType type, SDL_Renderer* renderer) {
    if (index < 0 || index >= (int)cells.size()) return false;
    Cell& cell = cells[index];
    if (cell.state != CellState::EMPTY) return false;

    const PlantSpec& spec = PlantFactory::getSpec(type);
    cell.plant = std::make_unique<Plant>(spec, renderer);
    cell.state = CellState::PLANTED;
    return true;
}

bool Grid::removePlant(int index) {
    if (index < 0 || index >= (int)cells.size()) return false;
    Cell& cell = cells[index];
    if (cell.state != CellState::PLANTED) return false;
    cell.plant.reset();
    cell.pests.clear();
    cell.state = CellState::EMPTY;
    return true;
}

float Grid::harvestPlant(int index) {
    if (index < 0 || index >= (int)cells.size()) return 0.f;
    Cell& cell = cells[index];
    if (!cell.plant || !cell.plant->isReadyToHarvest()) return 0.f;

    float money = cell.plant->harvest();
    cell.state = CellState::EMPTY;
    cell.plant.reset();
    cell.pests.clear();

    Sound::instance().play(SoundID::HARVEST, 0.25f);
    if (onHarvest) onHarvest(index, money);
    return money;
}

bool Grid::waterPlant(int index, float amount) {
    if (index < 0 || index >= (int)cells.size()) return false;
    Cell& cell = cells[index];
    if (!cell.plant) return false;
    cell.plant->water(amount);
    Sound::instance().play(SoundID::WATER_PLANT, 0.25f);
    return true;
}

bool Grid::applyFertilizer(int index, float amount) {
    if (index < 0 || index >= (int)cells.size()) return false;
    Cell& cell = cells[index];
    if (!cell.plant) return false;
    cell.plant->applyFertilizer(amount);
    return true;
}

bool Grid::applyPesticide(int index, float strength) {
    if (index < 0 || index >= (int)cells.size()) return false;
    Cell& cell = cells[index];
    for (auto& pest : cell.pests) {
        pest->applyPesticide(strength);
    }
    return true;
}

void Grid::spawnPest(PestType type, int cellIndex, SDL_Renderer* renderer) {
    if (cellIndex < 0 || cellIndex >= (int)cells.size()) return;
    Cell& cell = cells[cellIndex];
    if (cell.state != CellState::PLANTED || !cell.plant) return;

    PestSpec spec;
    spec.type = type;
    switch (type) {
        case PestType::APHID:
            // easy - targets mint and zinnia - 1 pesticide
            spec.name = "Aphid"; spec.spawnRate = 1.f;
            spec.damagePerTick = 4.f; spec.damageInterval = 4.f;
            spec.pesticideHp = 1.f;
            spec.spritePath = "assets/sprites/aphid.png"; break;
        case PestType::SPIDER_MITE:
            // medium - targets fern and rose - 2 pesticide
            spec.name = "Spider Mite"; spec.spawnRate = 0.8f;
            spec.damagePerTick = 7.f; spec.damageInterval = 3.f;
            spec.pesticideHp = 2.f;
            spec.spritePath = "assets/sprites/spider_mite.png"; break;
        case PestType::MEALYBUG:
            // hard - tagets cactus and sunflower - 3 pesticide
            spec.name = "Mealybug"; spec.spawnRate = 0.6f;
            spec.damagePerTick = 12.f; spec.damageInterval = 2.5f;
            spec.pesticideHp = 3.f;
            spec.spritePath = "assets/sprites/mealybug.png"; break;
    }
    cell.pests.push_back(std::make_unique<Pest>(spec, cellIndex, renderer));
    Sound::instance().play(SoundID::PEST_SPAWN, 0.25f);
}

void Grid::checkPests(float /*dt*/) {
    // damage done in update(), use for more logic
}

bool Grid::unlockCell(int index) {
    if (index < 0 || index >= (int)cells.size()) return false;
    if (cells[index].state != CellState::LOCKED) return false;
    cells[index].state = CellState::EMPTY;
    return true;
}

void Grid::moveCursor(int dx, int dy) {
    int col = cursorIndex % cols + dx;
    int row = cursorIndex / cols + dy;
    col = std::max(0, std::min(cols - 1, col));
    row = std::max(0, std::min(rows - 1, row));
    cursorIndex = row * cols + col;
    Sound::instance().play(SoundID::CURSOR_MOVE, 0.25f);
}

Cell* Grid::getCell(int index) {
    if (index < 0 || index >= (int)cells.size()) return nullptr;
    return &cells[index];
}

bool Grid::isCellEmpty(int index) const {
    if (index < 0 || index >= (int)cells.size()) return false;
    return cells[index].state == CellState::EMPTY;
}

bool Grid::isCellUnlocked(int index) const {
    if (index < 0 || index >= (int)cells.size()) return false;
    return cells[index].state != CellState::LOCKED;
}
