

add_executable(pico_03 pico_three.cpp)
target_compile_features(pico_03 PRIVATE cxx_std_20)

# Set include path for pico.h
target_include_directories(pico_03 PRIVATE ../../src)

target_link_libraries(pico_03 core document graphics uix)

add_dependencies(pico_03 core document graphics uix)

set_property(TARGET pico_03 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_03")