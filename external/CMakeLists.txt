#find_package(Vulkan REQUIRED)
set(GLM_INCLUDE_DIR "./glm")
set(GLFW_INCLUDE_DIR "./glfw/include")
set(TINY_OBJ_INCLUDE_DIR "./tinyobj")
set(VMA_INCLUDE_DIR "./vma")

set(INCLUDE_DIRS 
    ${GLM_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${TINY_OBJ_INCLUDE_DIR}
)
set(EXT_LIBS
    glfw
    tinyobjloader
    vk-bootstrap::vk-bootstrap
    meshoptimizer
    volk
    GPUOpen::VulkanMemoryAllocator
    tinygltf
)
#add directories by their names on current computer
add_subdirectory(GLFW)
add_subdirectory(glm)
add_subdirectory(tinyobj)
add_subdirectory(vk-bootstrap)
add_subdirectory(meshoptimizer)
add_subdirectory(volk)
add_subdirectory(vma)
add_subdirectory(tinygltf)
add_library(ExternalLibs INTERFACE)
target_link_libraries(ExternalLibs INTERFACE ${EXT_LIBS})
target_include_directories(ExternalLibs INTERFACE ${INCLUDE_DIRS}) 