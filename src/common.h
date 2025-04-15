#pragma once
#if defined(_WIN32)
#   define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) || defined(__unix__)
#   define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
#   define VK_USE_PLATFORM_MACOS_MVK
#else
#endif
#define VK_NO_PROTOTYPES
#include "volk.h"
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
