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
#include "camera.hpp"

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
    Camera camera;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "coah-engine", nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, Engine::mouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void initVulkan() {
        ctx.init(window);
        swapchain.init(ctx);
        renderer.init(&ctx, &swapchain, &camera);
    }

    void loop() {
        float lastTime = 0.0f;
        while (!glfwWindowShouldClose(window)) {
            float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            lastTime = currentTime;

            glfwPollEvents();
            camera.processInput(window, dt);
            renderer.updateUniformBuffer();
            renderer.drawFrame();
        }
        vkDeviceWaitIdle(ctx.device);
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        Engine* e = (Engine*)glfwGetWindowUserPointer(window);
        Camera& cam = e->camera;

        static float lastX = WIDTH / 2.0f;
        static float lastY = HEIGHT / 2.0f;

        if (cam.firstMouse) { 
            lastX = xpos; 
            lastY = ypos; 
            cam.firstMouse = false; 
        }

        float dx = (xpos - lastX) * cam.sensitivity;
        float dy = (lastY - ypos) * cam.sensitivity;
        lastX = xpos; 
        lastY = ypos;

        cam.yaw   += dx;
        cam.pitch  = glm::clamp(cam.pitch + dy, -89.0f, 89.0f);

        cam.front = glm::normalize(glm::vec3(
            cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch)),
            sin(glm::radians(cam.pitch)),
            sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch))
        ));
    }

    void cleanup() {
        renderer.cleanup();
        swapchain.cleanup(ctx);
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