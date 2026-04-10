# wrap_hlsl_guard.cmake
# Wraps the _source definition in a scribe-generated .cpp with #ifndef __APPLE__
# Usage: cmake -DFILE=<path.cpp> -DCLASS=<ClassName> -P wrap_hlsl_guard.cmake

file(READ "${FILE}" content)
string(FIND "${content}" "const std::string ${CLASS}::_source" pos)
if(pos GREATER -1)
    string(REPLACE
        "const std::string ${CLASS}::_source"
        "#ifndef __APPLE__\nconst std::string ${CLASS}::_source"
        content "${content}")
    # Add closing #endif at the very end
    string(APPEND content "\n#endif // !__APPLE__\n")
    file(WRITE "${FILE}" "${content}")
endif()
