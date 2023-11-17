#pragma once
//#include <vulkan/vulkan.h>
//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
class HelloTriangleApplication {
public:
	void run();
private:
	GLFWwindow* m_window;
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanUp();
};