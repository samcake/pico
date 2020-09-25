

add_executable(pico_interop-gdi pico_interop-gdi.cpp)
target_compile_features(pico_interop-gdi PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_interop-gdi PRIVATE ../../src)

target_link_libraries(pico_interop-gdi pico)
