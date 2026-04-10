add_executable(pico_job pico_job.cpp)
target_compile_features(pico_job PRIVATE cxx_std_20)
target_include_directories(pico_job PRIVATE ../../src)
target_link_libraries(pico_job core)

set_property(TARGET pico_job PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_job")
