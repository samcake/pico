
#---------------------------------------------
# Graphics Library
#---------------------------------------------

file(GLOB GRAPHICS_SHADERS
    "gpu/*.hlsl"
    "render/*.hlsl"
    "drawables/*.hlsl"
)

# for each shaders, pre compile them and add the generated  .h and .cpp to the build
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

if(APPLE)
    find_program(GLSLANG_EXE glslangValidator)
    find_program(SPIRV_CROSS_EXE spirv-cross)
    set(HLSL2MSL_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/hlsl2msl.cmake)
    set(HLSL_INCLUDE_DIR1 "${CMAKE_CURRENT_SOURCE_DIR}/render")
    set(HLSL_INCLUDE_DIR2 "${CMAKE_CURRENT_SOURCE_DIR}/drawables")
endif()

foreach( item IN LISTS GRAPHICS_SHADERS )
    get_filename_component(item_name ${item} NAME_WE)
    get_filename_component(item_ext ${item} LAST_EXT)

    set(item_target_root ${SHADER_OUTPUT_DIR}/${item_name})

    list(APPEND INCLUDE_SHADERS_HPP ${item_target_root}.h)
    list(APPEND INCLUDE_SHADERS_CPP ${item_target_root}.cpp)

    set_source_files_properties(${item} PROPERTIES VS_TOOL_OVERRIDE "None")
    add_custom_command(
        DEPENDS scribe
        MAIN_DEPENDENCY ${item}
        OUTPUT ${item_target_root}.h ${item_target_root}.cpp
        COMMAND scribe ${item} -c++ -parseFilenameType -o ${item_target_root}
    )

    # On Apple, convert non-include shaders from HLSL to MSL
    if(APPLE AND GLSLANG_EXE AND SPIRV_CROSS_EXE)
        # Determine shader stage from filename suffix
        string(REGEX MATCH "_(vert|frag|comp|ray|inc)" _stage_match "${item_name}")
        set(_stage "${CMAKE_MATCH_1}")

        if(_stage AND NOT _stage STREQUAL "inc" AND NOT _stage STREQUAL "ray")
            set(item_msl_cpp ${SHADER_OUTPUT_DIR}/${item_name}_msl.cpp)
            list(APPEND INCLUDE_SHADERS_CPP ${item_msl_cpp})

            # Wrap scribe HLSL source in #ifndef __APPLE__ so MSL version wins
            add_custom_command(
                DEPENDS ${item_target_root}.cpp
                OUTPUT ${item_target_root}.cpp.guarded
                COMMAND ${CMAKE_COMMAND}
                    -DFILE=${item_target_root}.cpp
                    -DCLASS=${item_name}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/wrap_hlsl_guard.cmake
                COMMAND ${CMAKE_COMMAND} -E touch ${item_target_root}.cpp.guarded
                COMMENT "Guard HLSL: ${item_name}"
            )

            # Convert HLSL → SPIR-V → MSL
            add_custom_command(
                DEPENDS ${item} ${HLSL2MSL_SCRIPT} ${item_target_root}.cpp.guarded
                OUTPUT ${item_msl_cpp}
                COMMAND ${CMAKE_COMMAND}
                    -DSHADER=${item}
                    -DCLASS=${item_name}
                    -DSTAGE=${_stage}
                    "-DINCLUDE_DIR1=${HLSL_INCLUDE_DIR1}"
                    "-DINCLUDE_DIR2=${HLSL_INCLUDE_DIR2}"
                    -DOUTPUT=${item_msl_cpp}
                    -DGLSLANG=${GLSLANG_EXE}
                    -DSPIRV_CROSS=${SPIRV_CROSS_EXE}
                    -P ${HLSL2MSL_SCRIPT}
                COMMENT "HLSL->MSL: ${item_name}"
            )
        endif()
    endif()

endforeach()

file(GLOB GRAPHICS_HEADERS
    "*.h"
    "gpu/*.h"
    "render/*.h"
    "drawables/*.h"
)

file(GLOB GRAPHICS_SOURCES
    "*.cpp"
    "gpu/*.cpp"
    "render/*.cpp"
    "drawables/*.cpp"
)

if(WIN32)
    file(GLOB D3D12_HEADERS "d3d12/*.h")
    file(GLOB D3D12_SOURCES "d3d12/*.cpp")
    list(APPEND GRAPHICS_HEADERS ${D3D12_HEADERS})
    list(APPEND GRAPHICS_SOURCES ${D3D12_SOURCES})
    source_group(graphics\\d3d12 REGULAR_EXPRESSION d3d12/*)
endif()

if(APPLE)
    file(GLOB METAL_HEADERS "metal/*.h")
    file(GLOB METAL_SOURCES "metal/*.mm")
    list(APPEND GRAPHICS_HEADERS ${METAL_HEADERS})
    list(APPEND GRAPHICS_SOURCES ${METAL_SOURCES})
    source_group(graphics\\metal REGULAR_EXPRESSION metal/*)

    set(GRAPHICS_LIBRARIES
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
    )
endif()

list(APPEND GRAPHICS_HEADERS ${INCLUDE_SHADERS_HPP})
list(APPEND GRAPHICS_SOURCES ${INCLUDE_SHADERS_CPP})


add_library(graphics
    ${GRAPHICS_HEADERS}
    ${GRAPHICS_SOURCES}
    ${GRAPHICS_SHADERS}
)


target_link_libraries(graphics
    ${GRAPHICS_LIBRARIES}
    ${GRAPHICS_API_LIBRARIES}
    core document
)

add_dependencies(graphics core)
add_dependencies(graphics document)
add_dependencies(graphics scribe)

# in graphics library we want the include path to start in 'this' folder
target_include_directories(graphics PUBLIC "." ${SHADER_OUTPUT_DIR} PRIVATE "."  )

target_compile_features(graphics PRIVATE cxx_std_20)

# source_group(graphics\\shaders FILES ${GRAPHICS_SHADERS})
source_group(graphics\\shaders\\generated REGULAR_EXPRESSION ${SHADER_OUTPUT_DIR}/*)

source_group(graphics\\render REGULAR_EXPRESSION render/*)
source_group(graphics\\gpu REGULAR_EXPRESSION gpu/*)
source_group(graphics\\drawables REGULAR_EXPRESSION drawables/*)
source_group(graphics REGULAR_EXPRESSION ./*)
