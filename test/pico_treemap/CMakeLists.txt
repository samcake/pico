

add_executable(pico_treemap pico_treemap.cpp)
target_compile_features(pico_treemap PRIVATE cxx_std_20)

# Set include path for pico.h
target_include_directories(pico_treemap PRIVATE ../../src)

target_link_libraries(pico_treemap graphics uix)

set_property(TARGET pico_treemap PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_treemap")
