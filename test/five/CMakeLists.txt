
#include(../../cmake/scribe_shader.cmake)

file(GLOB SHADER_FILES
    "*.hlsl"
)


# for each shaders, pre compile them and add the generated  .h and .cpp to the build
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

 foreach( item IN LISTS SHADER_FILES )
    get_filename_component(item_name ${item} NAME_WE)
    set(item_target_root ${SHADER_OUTPUT_DIR}/${item_name})

    list(APPEND INCLUDE_SHADERS_HPP ${item_target_root}.h)
    list(APPEND INCLUDE_SHADERS_CPP ${item_target_root}.cpp)

    set_source_files_properties(${item} PROPERTIES VS_TOOL_OVERRIDE "None")
    add_custom_command(
        DEPENDS ${item}
        MAIN_DEPENDENCY ${item}
        OUTPUT ${item_target_root}.h ${item_target_root}.cpp
        COMMAND scribe ${item} -c++ -o ${item_target_root}
    )
 
endforeach()

add_executable(pico_five pico_five.cpp ${SHADER_FILES} ${INCLUDE_SHADERS_CPP})


# Set include path for pico.h
target_include_directories(pico_five PRIVATE ../../src ${SHADER_OUTPUT_DIR})

target_link_libraries(pico_five pico)
