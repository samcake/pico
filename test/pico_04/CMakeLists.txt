

add_executable(pico_04 pico_four.cpp)
target_compile_features(pico_04 PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_04 PRIVATE ../../src)

target_link_libraries(pico_04 graphics uix)


set_property(TARGET pico_04 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_04")