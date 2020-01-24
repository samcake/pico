

add_executable(pico_five pico_five.cpp)

# Set include path for pico.h
target_include_directories(pico_five PRIVATE ../../src)

target_link_libraries(pico_five pico)
