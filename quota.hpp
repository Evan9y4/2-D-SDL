#include "engine.hpp"
#include "scene.hpp"
#include <SDL3/SDL.h>

std::vector<SDL_Event> Engine::keyEvents;
std::vector<SDL_Event> Engine::mouseEvents;

Engine::Engine() {}

Engine::~Engine() {
    shutdown();
}

bool Engine::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Bloom & Bloom", screenWidth, screenHeight, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Enable pixel-art scaling (nearest neighbor)
    SDL_SetRenderLogicalPresentation(renderer, screenWidth, screenHeight,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(renderer, 0);

    SDL_Log("Engine initialized successfully.");
    return true;
}

void Engine::run() {
    if (!scene) {
        SDL_Log("No scene set. Aborting run.");
        return;
    }

    running = true;
    Uint64 lastTime = SDL_GetTicks();

    while (running) {
        Uint64 now = SDL_GetTicks();
        float  dt  = (float)(now - lastTime) / 1000.0f; // seconds
        lastTime   = now;

        // event polling
        keyEvents.clear();
        mouseEvents.clear();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                SDL_Log("Quit event received.");
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                keyEvents.push_back(event);
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                event.type == SDL_EVENT_MOUSE_BUTTON_UP   ||
                event.type == SDL_EVENT_MOUSE_MOTION) {
                mouseEvents.push_back(event);
            }
        }

        scene->update(dt);

        // render
        SDL_SetRenderDrawColor(renderer, 20, 30, 15, 255);
        SDL_RenderClear(renderer);
        scene->render(renderer);
        SDL_RenderPresent(renderer);

        // cap frames
        Uint64 elapsed = SDL_GetTicks() - now;
        if (elapsed < (Uint64)TARGET_FRAME_TIME) {
            SDL_Delay((Uint32)(TARGET_FRAME_TIME - elapsed));
        }
    }
}

void Engine::shutdown() {
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window)   { SDL_DestroyWindow(window);     window   = nullptr; }
    SDL_Quit();
    SDL_Log("Engine shut down.");
}
