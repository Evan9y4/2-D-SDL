#include "engine.hpp"
#include "game_scene.hpp"
#include <SDL3/SDL.h>
#include <memory>

int main(int /*argc*/, char* /*argv*/[]) {
    Engine& engine = Engine::instance();

    if (!engine.init()) {
        SDL_Log("Engine initialization failed.");
        return 1;
    }

    auto scene = std::make_unique<GameScene>();
    if (!scene->init(engine.getRenderer())) {
        SDL_Log("GameScene initialization failed.");
        engine.shutdown();
        return 1;
    }

    engine.setScene(scene.get());
    engine.run();

    scene->shutdown();
    engine.shutdown();

    return 0;
}
