
#---------------------------------------------
# uix Library
#---------------------------------------------

file(GLOB UIX_HEADERS
    "imgui/backends/*.h"
    "imgui/*.h"
    "*.h"
)

file(GLOB UIX_SOURCES
    "imgui/backends/*.cpp"
    "imgui/*.cpp"
    "*.cpp"
)

add_library (uix
    ${UIX_HEADERS}
    ${UIX_SOURCES}
)


target_link_libraries(uix
    ${UIX_LIBRARIES}
    graphics core
)

add_dependencies(uix graphics core)

target_include_directories(uix PUBLIC "..")
target_compile_features(uix PRIVATE cxx_std_17)

source_group(uix\\imgui\\backends REGULAR_EXPRESSION imgui/backends/*)
source_group(uix\\imgui REGULAR_EXPRESSION imgui/*)
source_group(uix REGULAR_EXPRESSION ./*)

