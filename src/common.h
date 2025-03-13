#pragma once
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <set>
#include <array>
#define VK_CHECK(vkcommand, failedMessage) \
do{\
if((vkcommand)!=VK_SUCCESS){\
   throw std::runtime_error(\
failedMessage);\
}\
}while(0)

#define CHECK_TRUE(condition, failedMessage) \
do{\
if(!(condition)){\
   throw std::runtime_error(\
failedMessage);\
}\
}while(0)