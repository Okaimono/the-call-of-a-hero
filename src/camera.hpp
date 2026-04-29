#pragma once
#include "vulkan_includes.hpp"
#include "core/config.hpp"

struct Camera {
public:
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = -135.0f;
    float pitch = -20.0f;
    float speed = 20.0f;
    float sensitivity = 0.1f;
    bool firstMouse = true;

    void processInput(GLFWwindow* window, float dt) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += speed * dt * front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= speed * dt * front;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= glm::normalize(glm::cross(front, up)) * speed * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += glm::normalize(glm::cross(front, up)) * speed * dt;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      position += up * speed * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) position -= up * speed * dt;
    }
};