

add_executable(pico_terrain pico_terrain.cpp terrain.cpp terrain.h)
target_compile_features(pico_terrain PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_terrain PRIVATE ../../src)

target_link_libraries(pico_terrain graphics uix)
