#pragma once
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>

#define VK_CHECK(vkcommand, message) \
do{\
if((vkcommand)!=VK_SUCCESS){\
   throw std::runtime_error(\
#message);\
}\
}while(0)

#define CHECK_TRUE(condition, failedMessage) \
do{\
if(!(condition)){\
   throw std::runtime_error(\
failedMessage);\
}\
}while(0)