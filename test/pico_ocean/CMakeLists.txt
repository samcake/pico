

add_executable(pico_ocean pico_ocean.cpp ocean.cpp ocean.h)
target_compile_features(pico_ocean PRIVATE cxx_std_20)

# Set include path for pico.h
target_include_directories(pico_ocean PRIVATE ../../src)

target_link_libraries(pico_ocean graphics uix)

set_property(TARGET pico_ocean PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_ocean")
