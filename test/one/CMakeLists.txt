

add_executable(pico_one pico_one.cpp)

# Set include path for pico.h
target_include_directories(pico_one PRIVATE ../../src)

target_link_libraries(pico_one pico)
