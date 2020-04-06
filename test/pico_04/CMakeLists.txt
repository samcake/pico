

add_executable(pico_04 pico_four.cpp)

# Set include path for pico.h
target_include_directories(pico_04 PRIVATE ../../src)

target_link_libraries(pico_04 pico)
