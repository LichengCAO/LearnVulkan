function(funcAddToFolder target folder)
    set_property(TARGET ${target} PROPERTY FOLDER ${folder})
endfunction()

file(GLOB SHADER_SOURCES
    "shaders/*.vert"
    "shaders/*.frag"
    "shaders/*.comp"
    "shaders/*.tesc"
    "shaders/*.tese"
)

file(GLOB IMAGES
    "images/*.png"
    "images/*.jpg"
    "images/*.hdr"
    "images/*.bmp"
)

#copy images
foreach(IMAGE ${IMAGES})
   get_filename_component(fname ${IMAGE} NAME)
   configure_file(${IMAGE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/images/${fname} COPYONLY)
endforeach()

# add to the folder ~ in IDE explorer
source_group("ShaderSources" FILES ${SHADER_SOURCES})

foreach(SHADER ${SHADER_SOURCES})
    set(SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
    get_filename_component(fname ${SHADER} NAME)
    add_custom_target(${fname}.spv  
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DIR} &&
        $ENV{VK_SDK_PATH}/Bin/glslangValidator.exe -V ${SHADER} -o ${SHADER_DIR}/${fname}.spv -g
        SOURCES${SHADER}
    )
    # add to the folder CompliedSPV in IDE explorer
    funcAddToFolder("CompliedSPV" ${fname}.spv)
    add_dependencies(${CMAKE_PROJECT_NAME} ${fname}.spv)
endforeach()