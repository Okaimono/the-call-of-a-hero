#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <chrono>

#include "vulkan/vulkan_context.hpp"
#include "vulkan/swapchain.hpp"
#include "vulkan/renderer.hpp"
#include "core/config.hpp"
#include "game/coah.hpp"
#include "coah_engine/input_manager.hpp"

class CoahEngine {
public:
    void run() {
        initWindow();
        initVulkan();
        inputManager.init(window);
        loop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;

    VulkanContext ctx;
    Swapchain swapchain;
    Renderer renderer;
    CallOfAHero coah;
    InputManager inputManager;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "coah-engine", nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, CoahEngine::mouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void initVulkan() {
        ctx.init(window);
        swapchain.init(ctx);
        renderer.init(&ctx, &swapchain);
        coah.init(&renderer);
    }

    void loop() {
        float lastTime = 0.0f;
        while (!glfwWindowShouldClose(window)) {
            float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            lastTime = currentTime;

            glfwPollEvents();
            coah.update(&inputManager, dt);
            coah.render();
        }
        vkDeviceWaitIdle(ctx.device);
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        CoahEngine* e = static_cast<CoahEngine*>(glfwGetWindowUserPointer(window));
        e->coah.player.onMouseMove(xpos, ypos);
    }

    void cleanup() {
        renderer.cleanup();
        swapchain.cleanup(ctx);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};