find_package(Vulkan REQUIRED)
set(GLM_INCLUDE_DIR "./glm")
set(GLFW_INCLUDE_DIR "./glfw/include")
set(INCLUDE_DIRS 
    ${GLM_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
)
set(EXT_LIBS
    Vulkan::Vulkan
    glfw
)
add_subdirectory(glfw)
add_subdirectory(glm)
add_library(ExternalLibs INTERFACE)
target_link_libraries(ExternalLibs INTERFACE ${EXT_LIBS})
target_include_directories(ExternalLibs INTERFACE ${INCLUDE_DIRS}) 
    