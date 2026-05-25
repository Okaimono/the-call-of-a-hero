#pragma once

class PhysicsEngine {
public:
    void update(float dt, glm::vec3& position, glm::vec3& velocity, glm::vec3& acceleration) {
        acceleration = glm::vec3(0.0f, G, 0.0f);
        velocity += acceleration * dt;
        position += velocity * dt;
        std::cout << position.y << "\n";
    }
private:
    static constexpr float G = -9.8f;

};