cmake_minimum_required(VERSION 3.14)

project(scribe)

file(GLOB TARGET_SRCS src/*)
add_executable(scribe ${TARGET_SRCS})
if (WIN32)
    set_property(TARGET scribe APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()
install(TARGETS scribe RUNTIME DESTINATION scribe)

set(SCRIBE_EXE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scribe.exe" PARENT_SCOPE)
message( "Scribe is Scribe Folder: ${SCRIBE_EXE}")