
#---------------------------------------------
# Document Library
#---------------------------------------------

file(GLOB DOCUMENT_HEADERS
    "*.h"
)

file(GLOB DOCUMENT_SOURCES
    "*.cpp"
)

add_library (document
    ${DOCUMENT_HEADERS}
    ${DOCUMENT_SOURCES}
)

target_link_libraries(document
    PUBLIC core
)

add_dependencies(document core)

target_include_directories(document PUBLIC ".." PRIVATE "." )

target_compile_features(document PRIVATE cxx_std_17)

source_group(document REGULAR_EXPRESSION ./*)

