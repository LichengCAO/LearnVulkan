#pragma once
#define GLFW_INCLUDE_VULKAN //->auto load <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <set>
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