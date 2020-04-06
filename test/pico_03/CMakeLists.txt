

add_executable(pico_03 pico_three.cpp)

# Set include path for pico.h
target_include_directories(pico_03 PRIVATE ../../src)

target_link_libraries(pico_03 pico)
