

add_executable(pico_two pico_two.cpp)

# Set include path for pico.h
target_include_directories(pico_two PRIVATE ../../src)

target_link_libraries(pico_two pico)
