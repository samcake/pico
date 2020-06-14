cmake_minimum_required(VERSION 3.14)

project(Scribe)

file(GLOB TARGET_SRCS src/*)
add_executable(scribe ${TARGET_SRCS})
if (WIN32)
    set_property(TARGET scribe APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()
install(TARGETS scribe RUNTIME DESTINATION scribe)
