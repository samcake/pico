
#---------------------------------------------
# Graphics Library
#---------------------------------------------

#if(USE_D3D12 )
#    set(GRAPHICS_API_LIBRARIES "d3d12.lib dxgi.lib dxguid.lib D3DCompiler.lib Shlwapi.lib")  
#endif()


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
    add_custom_command(
        DEPENDS scribe
        MAIN_DEPENDENCY ${item}
        OUTPUT ${item_target_root}.h ${item_target_root}.cpp
        COMMAND scribe ${item} -c++ -parseFilenameType -o ${item_target_root} 
    )
 
endforeach()

#message( "this shader header: ${INCLUDE_SHADERS_HPP}" )
#message( "this shader bodies: ${INCLUDE_SHADERS_CPP}" )

file(GLOB GRAPHICS_HEADERS
    "*.h"
    "gpu/*.h"
    "d3d12/*.h"
    "vk/*.h"
    "render/*.h"
    "drawables/*.h"
)

list(APPEND  GRAPHICS_HEADERS   ${INCLUDE_SHADERS_HPP})

file(GLOB GRAPHICS_SOURCES
    "*.cpp"
    "gpu/*.cpp"
    "d3d12/*.cpp"
    "vk/*.cpp"
    "render/*.cpp"
    "drawables/*.cpp"
)

list(APPEND  GRAPHICS_SOURCES   ${INCLUDE_SHADERS_CPP})


add_library (graphics
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
target_include_directories(graphics PUBLIC "." PRIVATE "." ${SHADER_OUTPUT_DIR} )

target_compile_features(graphics PRIVATE cxx_std_17)

# source_group(graphics\\shaders FILES ${GRAPHICS_SHADERS})
source_group(graphics\\shaders\\generated REGULAR_EXPRESSION ${SHADER_OUTPUT_DIR}/*)

source_group(graphics\\render REGULAR_EXPRESSION render/*)
source_group(graphics\\gpu REGULAR_EXPRESSION gpu/*)
source_group(graphics\\d3d12 REGULAR_EXPRESSION d3d12/*)
source_group(graphics\\vk REGULAR_EXPRESSION vk/*)
source_group(graphics\\drawables REGULAR_EXPRESSION drawables/*)
source_group(graphics REGULAR_EXPRESSION ./*)

