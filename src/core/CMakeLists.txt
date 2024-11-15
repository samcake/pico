
#---------------------------------------------
# Core Library
#---------------------------------------------

file(GLOB CORE_HEADERS
    "stl/*.h"
    "math/*.h"
    "json/*.h"
    "*.h"
)

file(GLOB CORE_SOURCES
    "*.cpp"
    "stl/*.cpp"
    "json/*.cpp"
)

add_library (core
    ${CORE_HEADERS}
    ${CORE_SOURCES}
)


# target_link_libraries(core
#    ${CORE_LIBRARIES}
#)

target_include_directories(core PUBLIC ".." PRIVATE "." )

target_compile_features(core PRIVATE cxx_std_20)

source_group(core\\stl REGULAR_EXPRESSION stl/*)
source_group(core\\math REGULAR_EXPRESSION math/*)
source_group(core\\json REGULAR_EXPRESSION json/*)
source_group(core REGULAR_EXPRESSION ./*)