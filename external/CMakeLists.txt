add_library(ExternalLibs INTERFACE)

#set builld option for 3rd party
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
set(TINYGLTF_INSTALL OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
set(SPIRV_REFLECT_EXECUTABLE OFF)
set(IMGUI_VULKAN ON)
set(IMGUI_GLFW ON)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
#find_package(Vulkan REQUIRED)
set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glm")
set(GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glfw/include")
set(TINY_OBJ_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tinyobj")
set(VMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vma")
set(SPIRV_REFLECT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect")
set(IMGUI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui" "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")
set(VDB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openvdb/openvdb/openvdb" "${CMAKE_CURRENT_SOURCE_DIR}/openvdb/nanovdb/nanovdb")
set(METIS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(INCLUDE_DIRS 
    ${GLM_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${TINY_OBJ_INCLUDE_DIR}
    ${SPIRV_REFLECT_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${OPENVDB_INCLUDE_DIRS}
    ${METIS_INCLUDE_DIR}
)

set(EXT_LIBS
    glfw
    glm
    tinyobjloader
    vk-bootstrap
    meshoptimizer
    volk
    VulkanMemoryAllocator
    tinygltf
    spirv-reflect-static
    imgui
    nanovdb
    enkiTS
)

#===================================
#        initialize imgui
#===================================
cmake_minimum_required(VERSION 3.15)
project(ImGui LANGUAGES CXX)

OPTION(IMGUI_VULKAN "vulkan" OFF)
OPTION(IMGUI_GLFW "glfw" OFF)

# Add all ImGui source files
file(GLOB IMGUI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp")
file(GLOB IMGUI_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h")
file(GLOB_RECURSE VULKAN_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*vulkan*.h")
file(GLOB_RECURSE VULKAN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*vulkan*.cpp")
file(GLOB_RECURSE GLFW_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*glfw*.h")
file(GLOB_RECURSE GLFW_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*glfw*.cpp")
file(GLOB_RECURSE STD_STRING_FILES "${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.*")

if (IMGUI_VULKAN AND IMGUI_GLFW)
    set(IMGUI_SRC
        ${IMGUI_SOURCES}
        ${IMGUI_HEADERS}
        ${VULKAN_HEADERS}
        ${VULKAN_SOURCES}
        ${GLFW_HEADERS}
        ${GLFW_SOURCES}
        ${STD_STRING_FILES}
    )
endif()

# Create the ImGui library
add_library(imgui STATIC ${IMGUI_SRC})

# Include vulkan -- copy from volk
if(IMGUI_VULKAN)
  # Try an explicit CMake variable first, then any Vulkan paths
  # discovered by FindVulkan.cmake, then the $VULKAN_SDK environment
  # variable if nothing else works.
  if(VULKAN_HEADERS_INSTALL_DIR)
    set(IMGUI_INCLUDES "${VULKAN_HEADERS_INSTALL_DIR}/include")
  else()
    find_package(Vulkan QUIET)
    if(Vulkan_INCLUDE_DIRS)
      set(IMGUI_INCLUDES "${Vulkan_INCLUDE_DIRS}")
    elseif(DEFINED ENV{VULKAN_SDK})
      set(IMGUI_INCLUDES "$ENV{VULKAN_SDK}/include")
    endif()
  endif()

  if(IMGUI_INCLUDES)
    target_include_directories(imgui PUBLIC "${IMGUI_INCLUDES}")
  endif()
endif()

#GLFW_INCLUDE_DIR is set by parent CMakeLists.txt
if(IMGUI_GLFW)
    if(GLFW_INCLUDE_DIR)
        target_include_directories(imgui PUBLIC "${GLFW_INCLUDE_DIR}")
    endif()
endif()

# Include directories for public usage
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${IMGUI_INCLUDE_DIR}")

#define volk for imgui
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)
target_link_libraries(imgui PRIVATE volk)

# Set C++ standard
target_compile_features(imgui PUBLIC cxx_std_11)
#==================================
#      initialize imgui END
#==================================

# Setup openvdb
#find_package(OpenVDB CONFIG REQUIRED)

#find_package(metis CONFIG REQUIRED)

#add directories by their names on current computer
add_subdirectory(GLFW)
add_subdirectory(glm)
add_subdirectory(tinyobj)
add_subdirectory(vk-bootstrap)
add_subdirectory(meshoptimizer)
add_subdirectory(volk)
add_subdirectory(vma)
add_subdirectory(tinygltf)
add_subdirectory(spirv_reflect)
add_subdirectory(openvdb)
add_subdirectory(enkiTS)
target_link_libraries(ExternalLibs INTERFACE ${EXT_LIBS} metis)
target_include_directories(ExternalLibs INTERFACE ${INCLUDE_DIRS}) 

set_target_properties(${EXT_LIBS} PROPERTIES FOLDER "ThirdParty")
