#include "coah/engine.hpp"

void Engine::init(Application* app) {
    application = app;
    
}

void Engine::run() {
    running = true;
    while (running) {
        // tick
    }
}

void Engine::shutdown() {
    running = false;
}