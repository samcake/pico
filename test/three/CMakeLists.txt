

add_executable(poco_three poco_three.cpp)

# Set include path for poco.h
target_include_directories(poco_three PRIVATE ../../src)

target_link_libraries(poco_three poco)
