

add_executable(pico_02 pico_two.cpp)
target_compile_features(pico_02 PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_02 PRIVATE ../../src)

target_link_libraries(pico_02 graphics uix)

set_property(TARGET pico_02 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_02")
