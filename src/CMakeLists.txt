#---------------------------------------------
# Graphics Library
#---------------------------------------------

#if(USE_D3D12 )
#    set(GRAPHICS_LIBRARIES "d3d12.lib dxgi.lib dxguid.lib D3DCompiler.lib Shlwapi.lib")  
#endif()


#---------------------------------------------
# Poco Library
#---------------------------------------------

include_directories("poco/")

file(GLOB POCO_HEADERS
    "poco/*.h"
    "poco/gpu/*.h"
    "poco/d3d12/*.h"
)

file(GLOB POCO_SOURCES
    "poco/*.cpp"
    "poco/gpu/*.cpp"
    "poco/d3d12/*.cpp"
)

add_library (poco
    ${POCO_HEADERS}
    ${POCO_SOURCES}
)

target_link_libraries(poco
    ${POCO_LIBRARIES}
    ${GRAPHICS_LIBRARIES}
)

source_group(poco\\gpu REGULAR_EXPRESSION poco/gpu/*)
source_group(poco\\d3d12 REGULAR_EXPRESSION poco/d3d12/*)
source_group(poco REGULAR_EXPRESSION poco/*)

