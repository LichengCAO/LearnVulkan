#set builld option for 3rd party
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
set(TINYGLTF_INSTALL OFF)
set(SPIRV_REFLECT_STATIC_LIB ON)
set(SPIRV_REFLECT_EXECUTABLE OFF)

#find_package(Vulkan REQUIRED)
set(GLM_INCLUDE_DIR "./glm")
set(GLFW_INCLUDE_DIR "./glfw/include")
set(TINY_OBJ_INCLUDE_DIR "./tinyobj")
set(VMA_INCLUDE_DIR "./vma")
set(SPIRV_REFLECT_INCLUDE_DIR "./spirv_reflect")

set(INCLUDE_DIRS 
    ${GLM_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${TINY_OBJ_INCLUDE_DIR}
    ${SPIRV_REFLECT_INCLUDE_DIR}
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
add_subdirectory(spirv_reflect)
add_library(ExternalLibs INTERFACE)
target_link_libraries(ExternalLibs INTERFACE ${EXT_LIBS})
target_include_directories(ExternalLibs INTERFACE ${INCLUDE_DIRS}) 

set_target_properties(${EXT_LIBS} PROPERTIES FOLDER "ThirdParty")