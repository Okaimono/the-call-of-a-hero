#pragma once
#include "vulkan_includes.hpp"

class InputManager {
public:
    void init(GLFWwindow* window) {
        this->window = window;
    }

    bool keyPressed(int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

private:
    GLFWwindow* window;
};