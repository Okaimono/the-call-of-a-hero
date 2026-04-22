#pragma once
#include "../vulkan_includes.hpp"
#include "../core/config.hpp"

struct Camera {
    glm::vec3 position  = glm::vec3(0.0f, 80.0f, 3.0f);
    glm::vec3 front     = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up        = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw           = -90.0f;
    float pitch         = 0.0f;
    float speed         = 20.0f;
    float sensitivity   = 0.1f;
    bool firstMouse     = true;
    float lastX         = Config::WIDTH / 2.0f;
    float lastY         = Config::HEIGHT / 2.0f;
};