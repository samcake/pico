

add_executable(pico_02 pico_two.cpp)

# Set include path for pico.h
target_include_directories(pico_02 PRIVATE ../../src)

target_link_libraries(pico_02 pico)
