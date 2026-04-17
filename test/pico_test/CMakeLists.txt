add_executable(pico_test pico_test.cpp)
target_compile_features(pico_test PRIVATE cxx_std_20)
target_include_directories(pico_test PRIVATE ../../src)
target_link_libraries(pico_test core)

set_property(TARGET pico_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_test")
