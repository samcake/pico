

add_executable(pico_01 pico_one.cpp)
target_compile_features(pico_01 PRIVATE cxx_std_20)

# Set include path for pico.h
target_include_directories(pico_01 PRIVATE ../../src)

target_link_libraries(pico_01 graphics uix)

# Working directory for running from test folder (asset paths are relative to it)
set_property(TARGET pico_01 PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_01")
