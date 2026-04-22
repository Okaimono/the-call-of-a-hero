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

class Engine {
public:
    void run() {
        initWindow();
        initVulkan();
        loop();
        cleanup();
    }
    
private:
    GLFWwindow* window = nullptr;

    VulkanContext ctx;
    Swapchain swapchain;
    Renderer renderer;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "coah-engine", nullptr, nullptr);
    }

    void initVulkan() {
        ctx.init(window);
        swapchain.init(ctx);
        renderer.init(&ctx, &swapchain);
    }

    void loop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            renderer.updateUniformBuffer();
            renderer.drawFrame();
        }
        vkDeviceWaitIdle(ctx.device);
    }

    void cleanup() {
        renderer.cleanup();
        swapchain.cleanup(ctx);
        renderer.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Engine engine;
    try { engine.run(); }
    catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}