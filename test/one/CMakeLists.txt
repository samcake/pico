

add_executable(poco_one poco_one.cpp)

# Set include path for poco.h
target_include_directories(poco_one PRIVATE ../../src)

target_link_libraries(poco_one poco)
