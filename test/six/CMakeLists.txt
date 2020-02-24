

add_executable(pico_six pico_six.cpp)

# Set include path for pico.h
target_include_directories(pico_six PRIVATE ../../src)

target_link_libraries(pico_six pico)
