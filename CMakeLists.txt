cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MOUDULE_PATH ${CMAKE_MOUDULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(learn_vulkan)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

##set output directory for realse and debug for visual studio
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
endforeach()

#show folder in IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#add_subdirectory(src)

file(GLOB SOURCES
     "src/*.cpp"
)
file(GLOB HEADERS
    "src/*.h"
    "src/*.hpp"
)
source_group("Sources" FILES ${SOURCES})
source_group("Headers" FILES ${HEADERS})
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS})

add_subdirectory(external)
add_subdirectory(res)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ExternalLibs)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC Vulkan_INCLUDE_DIR)

