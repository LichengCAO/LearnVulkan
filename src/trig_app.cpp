#include "trig_app.h"
void HelloTriangleApplication::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanUp();
}

void HelloTriangleApplication::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
}

void HelloTriangleApplication::initVulkan()
{
}

void HelloTriangleApplication::mainLoop()
{
}

void HelloTriangleApplication::cleanUp()
{
}
