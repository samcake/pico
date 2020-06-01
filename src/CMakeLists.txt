
#---------------------------------------------
# Find Scribe
#---------------------------------------------
set(SCRIBE_EXE  "C:/Build/pico/bin/Debug/scribe.exe")
message( "Scribe is Pico Folder: ${SCRIBE_EXE}")
#---------------------------------------------
# Graphics Library
#---------------------------------------------

#if(USE_D3D12 )
#    set(GRAPHICS_LIBRARIES "d3d12.lib dxgi.lib dxguid.lib D3DCompiler.lib Shlwapi.lib")  
#endif()


#---------------------------------------------
# Pico Library
#---------------------------------------------
file(GLOB PICO_SHADERS
    "pico/drawables/*.hlsl"
)

# for each shaders, pre compile them and add the generated  .h and .cpp to the build
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

 foreach( item IN LISTS PICO_SHADERS )
    get_filename_component(item_name ${item} NAME_WE)
    set(item_target_root ${SHADER_OUTPUT_DIR}/${item_name})

    list(APPEND INCLUDE_SHADERS_HPP ${item_target_root}.h)
    list(APPEND INCLUDE_SHADERS_CPP ${item_target_root}.cpp)

    set_source_files_properties(${item} PROPERTIES VS_TOOL_OVERRIDE "None")
    add_custom_command(
        DEPENDS ${item}
        MAIN_DEPENDENCY ${item}
        OUTPUT ${item_target_root}.h ${item_target_root}.cpp
        COMMAND ${SCRIBE_EXE} ${item} -c++ -o ${item_target_root}
    )
 
endforeach()

message( "this shader header: ${INCLUDE_SHADERS_HPP}" )
message( "this shader bodies: ${INCLUDE_SHADERS_CPP}" )

file(GLOB PICO_HEADERS
    "pico/*.h"
    "pico/core/*.h"
    "pico/gpu/*.h"
    "pico/d3d12/*.h"
    "pico/render/*.h"
    "pico/drawables/*.h"
    "pico/content/*.h"
    "pico/window/*.h"
)

list(APPEND  PICO_HEADERS   ${INCLUDE_SHADERS_HPP})

file(GLOB PICO_SOURCES
    "pico/*.cpp"
    "pico/core/*.cpp"
    "pico/gpu/*.cpp"
    "pico/d3d12/*.cpp"
    "pico/render/*.cpp"
    "pico/drawables/*.cpp"
    "pico/content/*.cpp"
    "pico/window/*.cpp"
)

list(APPEND  PICO_SOURCES   ${INCLUDE_SHADERS_CPP})


add_library (pico
    ${PICO_HEADERS}
    ${PICO_SOURCES}
    ${PICO_SHADERS}
)


target_link_libraries(pico
    ${PICO_LIBRARIES}
    ${GRAPHICS_LIBRARIES}
)

add_dependencies(pico scribe)

target_include_directories(pico PRIVATE "pico/" ${SHADER_OUTPUT_DIR} )

source_group(pico\\shaders FILES ${PICO_SHADERS})
source_group(pico\\shaders\\generated REGULAR_EXPRESSION ${SHADER_OUTPUT_DIR}/*)

source_group(pico\\core REGULAR_EXPRESSION pico/core/*)
source_group(pico\\content REGULAR_EXPRESSION pico/content/*)
source_group(pico\\render REGULAR_EXPRESSION pico/render/*)
source_group(pico\\gpu REGULAR_EXPRESSION pico/gpu/*)
source_group(pico\\d3d12 REGULAR_EXPRESSION pico/d3d12/*)
source_group(pico\\drawables REGULAR_EXPRESSION pico/drawables/*)
source_group(pico\\window REGULAR_EXPRESSION pico/window/*)
source_group(pico REGULAR_EXPRESSION pico/*)

