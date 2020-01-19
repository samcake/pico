

add_executable(poco_four poco_four.cpp)

# Set include path for poco.h
target_include_directories(poco_four PRIVATE ../../src)

target_link_libraries(poco_four poco)
