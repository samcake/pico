

add_executable(pico_01 pico_one.cpp)

# Set include path for pico.h
target_include_directories(pico_01 PRIVATE ../../src)

target_link_libraries(pico_01 pico)
