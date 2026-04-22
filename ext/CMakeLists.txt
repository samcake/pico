add_subdirectory( scribe )

if(PICO_PROFILER_PIX)
    set(_pix_dll "${CMAKE_CURRENT_SOURCE_DIR}/pix/bin/x64/WinPixEventRuntime.dll")
    foreach(_cfg ${CMAKE_CONFIGURATION_TYPES})
        file(COPY ${_pix_dll} DESTINATION "${CMAKE_BINARY_DIR}/bin/${_cfg}")
    endforeach()
    file(COPY ${_pix_dll} DESTINATION "${CMAKE_BINARY_DIR}/bin")
endif()