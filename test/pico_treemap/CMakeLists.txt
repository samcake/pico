


file(GLOB GRAPHICS_SHADERS
    "*.hlsl"
)
# for each shaders, pre compile them and add the generated  .h and .cpp to the build
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

 foreach( item IN LISTS GRAPHICS_SHADERS )
    get_filename_component(item_name ${item} NAME_WE)
    get_filename_component(item_ext ${item} LAST_EXT)

    set(item_target_root ${SHADER_OUTPUT_DIR}/${item_name})

    list(APPEND INCLUDE_SHADERS_HPP ${item_target_root}.h)
    list(APPEND INCLUDE_SHADERS_CPP ${item_target_root}.cpp)

    set_source_files_properties(${item} PROPERTIES VS_TOOL_OVERRIDE "None")
    add_custom_command(
        DEPENDS scribe
        MAIN_DEPENDENCY ${item}
        OUTPUT ${item_target_root}.h ${item_target_root}.cpp
        COMMAND scribe ${item} -c++ -parseFilenameType -o ${item_target_root} 
    )
 
endforeach()

add_executable(pico_treemap pico_treemap.cpp ${INCLUDE_SHADERS_HPP} ${INCLUDE_SHADERS_CPP})
target_compile_features(pico_treemap PRIVATE cxx_std_20)

# Set include path for pico.h
target_include_directories(pico_treemap PRIVATE ../../src)

target_link_libraries(pico_treemap graphics uix)

set_property(TARGET pico_treemap PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/pico_treemap")

# in graphics library we want the include path to start in 'this' folder
target_include_directories(pico_treemap PUBLIC "." PRIVATE "." ${SHADER_OUTPUT_DIR} )
