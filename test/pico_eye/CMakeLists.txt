

add_executable(pico_eye pico_eye.cpp)
target_compile_features(pico_eye PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_eye PRIVATE ../../src)

target_link_libraries(pico_eye graphics uix)

set_property(TARGET pico_eye PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_eye")