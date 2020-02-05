#---------------------------------------------
# Graphics Library
#---------------------------------------------

#if(USE_D3D12 )
#    set(GRAPHICS_LIBRARIES "d3d12.lib dxgi.lib dxguid.lib D3DCompiler.lib Shlwapi.lib")  
#endif()


#---------------------------------------------
# Pico Library
#---------------------------------------------

include_directories("pico/")

file(GLOB PICO_HEADERS
    "pico/*.h"
    "pico/gpu/*.h"
    "pico/d3d12/*.h"
    "pico/render/*.h"
    "pico/content/*.h"
    "pico/window/*.h"
)

file(GLOB PICO_SOURCES
    "pico/*.cpp"
    "pico/gpu/*.cpp"
    "pico/d3d12/*.cpp"
    "pico/render/*.cpp"
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

source_group(pico\\content REGULAR_EXPRESSION pico/content/*)
source_group(pico\\render REGULAR_EXPRESSION pico/render/*)
source_group(pico\\gpu REGULAR_EXPRESSION pico/gpu/*)
source_group(pico\\d3d12 REGULAR_EXPRESSION pico/d3d12/*)
source_group(pico\\window REGULAR_EXPRESSION pico/window/*)
source_group(pico REGULAR_EXPRESSION pico/*)

