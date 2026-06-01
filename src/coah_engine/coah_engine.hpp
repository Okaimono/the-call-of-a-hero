#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <cstdio>

#include "vulkan/vulkan_context.hpp"
#include "vulkan/swapchain.hpp"
#include "vulkan/renderer.hpp"
#include "vulkan/renderer_resource_manager.hpp"
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
    RendererResourceManager rendererResourceManager;

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
        rendererResourceManager.init(&ctx, &swapchain);
        renderer.init(&ctx, &swapchain, &rendererResourceManager);
        coah.init(&renderer, &rendererResourceManager);
    }

    void loop() {
        float lastTime = 0.0f;
        static float tickTime = 0.0f;
        while (!glfwWindowShouldClose(window)) {
            float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            tickTime += dt;
            lastTime = currentTime;

            if (tickTime >= 1.0f / 60.0f) {
                coah.tick(1.0f / 60.0f);
                tickTime -= 1.0f / 60.0f;
            }

            glfwPollEvents();

            coah.update(&inputManager, dt);

            //VulkanContext::profile("coah.render()", [&]() {coah.render();});
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
        rendererResourceManager.cleanup();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};