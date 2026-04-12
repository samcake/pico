cmake_minimum_required(VERSION 3.14)

project(Scribe)

file(GLOB TARGET_SRCS src/*)
add_executable(scribe ${TARGET_SRCS})
set_target_properties(scribe PROPERTIES CXX_STANDARD 20)

if(APPLE)
    # Find spirv-cross for SPIR-V → MSL conversion (HLSL → SPIR-V uses glslangValidator CLI)
    find_library(SPIRV_CROSS_CORE_LIB spirv-cross-core HINTS /opt/homebrew/lib)
    find_library(SPIRV_CROSS_GLSL_LIB spirv-cross-glsl HINTS /opt/homebrew/lib)
    find_library(SPIRV_CROSS_MSL_LIB spirv-cross-msl HINTS /opt/homebrew/lib)
    find_path(SPIRV_CROSS_INCLUDE_DIR spirv_cross/spirv_msl.hpp HINTS /opt/homebrew/include)

    if(SPIRV_CROSS_MSL_LIB AND SPIRV_CROSS_GLSL_LIB AND SPIRV_CROSS_CORE_LIB AND SPIRV_CROSS_INCLUDE_DIR)
        target_compile_definitions(scribe PRIVATE SCRIBE_HAS_MSL=1)
        target_include_directories(scribe PRIVATE ${SPIRV_CROSS_INCLUDE_DIR})
        target_link_libraries(scribe PRIVATE
            ${SPIRV_CROSS_MSL_LIB}
            ${SPIRV_CROSS_GLSL_LIB}
            ${SPIRV_CROSS_CORE_LIB}
        )
        message(STATUS "Scribe: MSL conversion enabled (spirv-cross + glslangValidator CLI)")
    else()
        message(STATUS "Scribe: MSL conversion disabled (spirv-cross not found)")
    endif()
endif()

if(WIN32)
    set_property(TARGET scribe APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()
install(TARGETS scribe RUNTIME DESTINATION scribe)
