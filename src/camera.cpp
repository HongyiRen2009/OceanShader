#include "camera.h"
#include <glm/vec3.hpp> // vec3
#include <glm/vec4.hpp> // vec4
#include <glm/ext/matrix_transform.hpp> // translate, rotate, scale
#include <glm/ext/matrix_clip_space.hpp> // perspective
#include <glm/ext/scalar_constants.hpp> // pi
#include <glm/gtc/matrix_access.hpp>

using namespace glm;

Camera* Camera::instance = nullptr;

void Camera::camera_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (Camera::instance) {
		Camera::instance->on_mouse_callback(xpos, ypos);
	}
}

Camera::Camera(GLFWwindow* window) : window(window) {
	instance = this;
	glfwSetCursorPosCallback(window, Camera::camera_mouse_callback);
}

void Camera::on_mouse_callback(double xpos, double ypos) {
	if (firstMouse) {
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);
		firstMouse = false;
	}
	float xoffset = static_cast<float>(xpos) - lastX;
	float yoffset = lastY - static_cast<float>(ypos); // reversed since y-coordinates range from bottom to top
	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	vec3 direction;
	direction.x = cos(radians(yaw)) * cos(radians(pitch));
	direction.y = sin(radians(pitch));
	direction.z = sin(radians(yaw)) * cos(radians(pitch));
	front = normalize(direction);
}

void Camera::update() {
	right = normalize(cross(front, vec3(0.0f, 1.0f, 0.0f)));
	up = normalize(cross(right, front));
	processKeyboardInput();
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

void Camera::processKeyboardInput() {
	float cameraSpeed = 4.0 * deltaTime; // adjust accordingly
	vec3 flattenedFront = normalize(vec3(front.x, 0.0f, front.z));
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		cameraSpeed *= 8.0f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		position += cameraSpeed * flattenedFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		position -= cameraSpeed * flattenedFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		position -= normalize(cross(flattenedFront, vec3(0, 1, 0))) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		position += normalize(cross(flattenedFront, vec3(0, 1, 0))) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		position.y += cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		position.y -= cameraSpeed;
}

mat4 Camera::getViewMatrix() const {
	return lookAt(position, position + front, up);
}

mat4 Camera::getProjectionMatrix(float aspectRatio) const {
	return perspective(radians(90.0f), aspectRatio, 0.1f, 1000.0f);
}

Frustum Camera::extractFrustum(float aspectRatio) {
	Frustum f;
	mat4 m = getProjectionMatrix(aspectRatio) * getViewMatrix();

	// Left
	f.planes[0] = glm::row(m, 3) + glm::row(m, 0);
	// Right
	f.planes[1] = glm::row(m, 3) - glm::row(m, 0);
	// Bottom
	f.planes[2] = glm::row(m, 3) + glm::row(m, 1);
	// Top
	f.planes[3] = glm::row(m, 3) - glm::row(m, 1);
	// Near
	f.planes[4] = glm::row(m, 3) + glm::row(m, 2);
	// Far
	f.planes[5] = glm::row(m, 3) - glm::row(m, 2);

	// Normalize planes
	for (int i = 0; i < 6; ++i)
		f.planes[i] /= glm::length(glm::vec3(f.planes[i]));

	return f;
}