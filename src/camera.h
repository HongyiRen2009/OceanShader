#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

struct Frustum {
    glm::vec4 planes[6];
};

class Camera {
public:
    glm::vec3 position{ 0.0f, 2.0f, 0.0f };
    glm::vec3 front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    Camera(GLFWwindow* window);
    ~Camera() = default;

    void update();
    void processKeyboardInput();
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    Frustum extractFrustum(float aspectRatio);

    // Mouse callback forwarding
    void on_mouse_callback(double xpos, double ypos);

    static Camera* instance;

private:
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float lastX = 400.0f, lastY = 300.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    bool firstMouse = true;

    GLFWwindow* window = nullptr;

    static void camera_mouse_callback(GLFWwindow* window, double xpos, double ypos);
};