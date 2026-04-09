
#---------------------------------------------
# uix Library
#---------------------------------------------

file(GLOB UIX_HEADERS
    "imgui/*.h"
    "*.h"
)

file(GLOB UIX_SOURCES
    "imgui/*.cpp"
    "*.cpp"
)

if(NOT WIN32)
    list(REMOVE_ITEM UIX_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_win32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_dx12.cpp"
    )
endif()

if(APPLE)
    list(APPEND UIX_SOURCES
        imgui/imgui_impl_metal.mm
        imgui/imgui_impl_osx.mm
        Imgui_mac.mm
        Window.mm
    )
    set_source_files_properties(
        imgui/imgui_impl_metal.mm
        imgui/imgui_impl_osx.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    set(UIX_LIBRARIES
        "-framework AppKit"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Foundation"
    )
endif()

add_library(uix
    ${UIX_HEADERS}
    ${UIX_SOURCES}
)


target_link_libraries(uix
    ${UIX_LIBRARIES}
    graphics core
)

add_dependencies(uix graphics core)

target_include_directories(uix PUBLIC "..")
target_compile_features(uix PRIVATE cxx_std_20)

source_group(uix\\imgui REGULAR_EXPRESSION imgui/*)
source_group(uix REGULAR_EXPRESSION ./*)
