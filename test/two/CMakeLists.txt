

add_executable(poco_two poco_two.cpp)

# Set include path for poco.h
target_include_directories(poco_two PRIVATE ../../src)

target_link_libraries(poco_two poco)
