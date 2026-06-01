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

    bool mouseClick() {
        static bool mouseWasDown = false;
        if (!mouseWasDown && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            mouseWasDown = true;
            return true;
        }
        if (mouseWasDown && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) {
            mouseWasDown = false;
        }
        return false;
    }

private:
    GLFWwindow* window;
};