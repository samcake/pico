

add_executable(pico_model pico_model.cpp)
target_compile_features(pico_model PRIVATE cxx_std_17)

# Set include path for pico.h
target_include_directories(pico_model PRIVATE ../../src)

target_link_libraries(pico_model graphics uix)

set_property(TARGET pico_model PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_model")
