
#---------------------------------------------
# uix Library
#---------------------------------------------

file(GLOB UIX_HEADERS
    "*.h"
)

file(GLOB UIX_SOURCES
    "*.cpp"
)

add_library (uix
    ${UIX_HEADERS}
    ${UIX_SOURCES}
)


target_link_libraries(uix
    ${UIX_LIBRARIES}
    pico core
)

add_dependencies(uix pico)

target_include_directories(uix PUBLIC "..")
target_compile_features(uix PRIVATE cxx_std_17)

source_group(uix REGULAR_EXPRESSION ./*)

