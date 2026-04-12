
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

foreach( item IN LISTS GRAPHICS_SHADERS )
    get_filename_component(item_name ${item} NAME_WE)
    get_filename_component(item_ext ${item} LAST_EXT)

    set(item_target_root ${SHADER_OUTPUT_DIR}/${item_name})

    list(APPEND INCLUDE_SHADERS_HPP ${item_target_root}.h)
    list(APPEND INCLUDE_SHADERS_CPP ${item_target_root}.cpp)

    set_source_files_properties(${item} PROPERTIES VS_TOOL_OVERRIDE "None")

    # On Apple, pass -msl to scribe for vert/frag/comp shaders (not inc/ray)
    set(_msl_flag "")
    if(APPLE)
        string(REGEX MATCH "_(vert|frag|comp)" _stage_match "${item_name}")
        if(_stage_match)
            set(_msl_flag "-msl")
        endif()
    endif()

    add_custom_command(
        DEPENDS scribe
        MAIN_DEPENDENCY ${item}
        OUTPUT ${item_target_root}.h ${item_target_root}.cpp
        COMMAND scribe ${item} -c++ -parseFilenameType ${_msl_flag}
            -I ${CMAKE_CURRENT_SOURCE_DIR}/render
            -I ${CMAKE_CURRENT_SOURCE_DIR}/drawables
            -o ${item_target_root}
    )

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
