

add_executable(pico_three pico_three.cpp)

# Set include path for pico.h
target_include_directories(pico_three PRIVATE ../../src)

target_link_libraries(pico_three pico)
