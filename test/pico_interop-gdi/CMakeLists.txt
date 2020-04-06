

add_executable(pico_interop-gdi pico_interop-gdi.cpp)

# Set include path for pico.h
target_include_directories(pico_interop-gdi PRIVATE ../../src)

target_link_libraries(pico_interop-gdi pico)
