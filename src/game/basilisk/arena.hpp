#pragma once
#include "game/basilisk/basilisk.hpp"
#include "game/basilisk/tower.hpp"
#include "game/player/player.hpp"
#include "game/basilisk/basilisk_phase.hpp"
#include "vulkan/renderer.hpp"

#include <memory>

enum class ArenaState {NONE, TOWER, BASILISK_FIRST, BASILISK_SECOND};

class Arena {
public:
    Player* player = nullptr;
    Basilisk* basilisk = nullptr;
    Renderer* renderer = nullptr;

    std::unique_ptr<BasiliskPhase> basiliskPhase;

    bool active = false;

    Tower tower;

    void init(Player* player, Basilisk* basilisk, Renderer* renderer) {
        this->player = player;
        this->basilisk = basilisk;
        this->renderer = renderer;

        prevState = ArenaState::NONE;
        state = ArenaState::TOWER;
        active = true;
    }

    void begin() {
        basiliskPhase = std::make_unique<BasiliskPhase1>();
        basiliskPhase->onEnter(basilisk);
    }

    void tick(float dt) {
        basiliskPhase->onTick(basilisk, dt);
    }

private:
    ArenaState prevState;
    ArenaState state;
};