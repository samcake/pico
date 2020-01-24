

add_executable(pico_four pico_four.cpp)

# Set include path for pico.h
target_include_directories(pico_four PRIVATE ../../src)

target_link_libraries(pico_four pico)
