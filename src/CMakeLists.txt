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
 foreach( item IN LISTS PICO_SHADERS )
    get_filename_component(shadername )
    if( item STREQUAL "" )
   # set(generated_file ${item})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${item}.h ${CMAKE_CURRENT_BINARY_DIR}/${item}.cpp
        COMMAND scribe ${item} -c++ -o ${CMAKE_CURRENT_BINARY_DIR}/${item}
    )
endforeach()


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

add_library (pico
    ${PICO_HEADERS}
    ${PICO_SOURCES}
)

target_link_libraries(pico
    ${PICO_LIBRARIES}
    ${GRAPHICS_LIBRARIES}
)

target_include_directories(pico PRIVATE "pico/")

source_group(pico\\core REGULAR_EXPRESSION pico/core/*)
source_group(pico\\content REGULAR_EXPRESSION pico/content/*)
source_group(pico\\render REGULAR_EXPRESSION pico/render/*)
source_group(pico\\gpu REGULAR_EXPRESSION pico/gpu/*)
source_group(pico\\d3d12 REGULAR_EXPRESSION pico/d3d12/*)
source_group(pico\\drawables REGULAR_EXPRESSION pico/drawables/*)
source_group(pico\\window REGULAR_EXPRESSION pico/window/*)
source_group(pico REGULAR_EXPRESSION pico/*)

