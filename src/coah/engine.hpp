#pragma once
#include "coah/application.hpp"

class Engine {
public:
    void init(Application* application);
    void run();
    void shutdown();

private:
    bool running = false;
    Application* application;
};