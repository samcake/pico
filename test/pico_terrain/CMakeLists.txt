

add_executable(pico_terrain pico_terrain.cpp terrain.cpp terrain.h)
target_compile_features(pico_terrain PRIVATE cxx_std_20)

# Set include path for pico.h
target_include_directories(pico_terrain PRIVATE ../../src)

target_link_libraries(pico_terrain graphics uix)

set_property(TARGET pico_terrain PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_terrain")
