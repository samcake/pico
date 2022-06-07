

add_executable(pico_ray pico_ray.cpp)
target_compile_features(pico_ray PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_ray PRIVATE ../../src)

target_link_libraries(pico_ray graphics uix)

set_property(TARGET pico_ray PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_ray")