#pragma once
#include "game/basilisk/basilisk.hpp"
#include "game/player/player.hpp"

/*
IDEAS FOR STAGES:

Stage 1:
- Basilisk spawns in tower.
- Make it rotate towards the outside of the arena
- Have it rotate left and right / slightly up
- Make it large, fast, and aggressive at the spawn start. Intimidate.
- No aggression yet.

Stage 2:
- Allow the boss to begin to charge towards the player.
- Camoflauge in the dark, with only its green eyes being able to be spotted.
- If you look at it in the eyes, it will charge you.
- When it charges you, then you can attack

*/

class BasiliskPhase {
public:
    virtual void onTick(Basilisk* basilisk, float dt) = 0;
    virtual void onEnter(Basilisk* basilisk) = 0;
    
};

class BasiliskPhase1 : public BasiliskPhase {
public:
    void onTick(Basilisk* basilisk, float dt) override {
        circles(basilisk, dt);
    };

    void onEnter(Basilisk* basilisk) override {
        basilisk->segments[0].pos = glm::vec3(0.0f, 60.0f, 0.0f);
    };

    void circles(Basilisk* basilisk, float dt) {
        static float timePassed = 0.0f;
        if (timePassed == 0.0f) {
            basilisk->headVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
            basilisk->segments[0].pos = glm::vec3 (0.0f, 60.0f, 10.0f);
        }
    }

};