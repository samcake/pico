// MslCompiler.h
// HLSL → SPIR-V → MSL compilation for scribe
//
// Uses glslangValidator CLI + spirv-cross C++ library.
// Only available when SCRIBE_HAS_MSL is defined (macOS with Homebrew libs).
//
#pragma once

#ifdef SCRIBE_HAS_MSL

#include <string>
#include <vector>

struct MslResult {
    bool success = false;
    std::string mslSource;
    std::string errorMessage;
};

// Compile HLSL source file to MSL.
// hlslFilePath: path to the .hlsl file (glslangValidator resolves #includes).
// hlslSource: flattened source for entry point detection (not passed to compiler).
// stage: "vert", "frag", or "comp"
// entryPoints: HLSL entry point function names to try (first success wins).
// includePaths: directories for #include resolution.
// ssboBindingShift: binding offset for StructuredBuffer (default 9).
MslResult compileHlslToMsl(
    const std::string& hlslFilePath,
    const std::string& hlslSource,
    const std::string& stage,
    const std::vector<std::string>& entryPoints,
    const std::vector<std::string>& includePaths,
    int ssboBindingShift = 9
);

#endif // SCRIBE_HAS_MSL
