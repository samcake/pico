// MslCompiler.cpp
// HLSL → SPIR-V → MSL compilation for scribe
//
// Uses glslangValidator CLI for HLSL→SPIR-V (the C++ API doesn't honor
// --shift-ssbo-binding for explicit register() bindings, see
// https://github.com/KhronosGroup/glslang/issues/794).
// Uses spirv-cross C++ library for SPIR-V→MSL.
//
#ifdef SCRIBE_HAS_MSL

#include "MslCompiler.h"

#include <spirv_cross/spirv_msl.hpp>
#include <spirv_cross/spirv_parser.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Init / shutdown (no-ops since we use glslangValidator CLI)
// ---------------------------------------------------------------------------
void mslCompilerInit() {}
void mslCompilerShutdown() {}

// ---------------------------------------------------------------------------
// Compile HLSL to SPIR-V via glslangValidator subprocess
// ---------------------------------------------------------------------------
static bool compileToSpirv(const std::string& hlslFile, const std::string& entryPoint,
                           const std::string& stage, const std::vector<std::string>& includePaths,
                           int ssboShift, std::vector<uint32_t>& outSpirv, std::string& outError) {
    char spvFile[] = "/tmp/scribe_spirv_XXXXXX";
    int fd = mkstemp(spvFile);
    if (fd >= 0) close(fd);

    std::string cmd = "glslangValidator -D -e " + entryPoint + " -S " + stage + " -V"
        + " --shift-ssbo-binding " + stage + " " + std::to_string(ssboShift);
    for (auto& p : includePaths) {
        if (!p.empty()) cmd += " -I" + p;
    }
    cmd += " -o " + std::string(spvFile) + " " + hlslFile + " 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        outError = "Failed to run glslangValidator";
        return false;
    }
    char buf[256];
    std::string output;
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
    int ret = pclose(pipe);

    if (ret != 0) {
        outError = output;
        unlink(spvFile);
        return false;
    }

    FILE* f = fopen(spvFile, "rb");
    if (!f) {
        outError = "Failed to read SPIR-V output";
        unlink(spvFile);
        return false;
    }
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    outSpirv.resize(len / 4);
    fread(outSpirv.data(), 4, outSpirv.size(), f);
    fclose(f);
    unlink(spvFile);

    return !outSpirv.empty();
}

// ---------------------------------------------------------------------------
// Convert SPIR-V to MSL via spirv-cross (in-process)
// ---------------------------------------------------------------------------
static bool convertToMsl(const std::vector<uint32_t>& spirv,
                         std::string& outMsl, std::string& outError) {
    try {
        spirv_cross::Parser parser(spirv.data(), spirv.size());
        parser.parse();

        spirv_cross::CompilerMSL compiler(std::move(parser.get_parsed_ir()));

        spirv_cross::CompilerMSL::Options opts;
        opts.platform = spirv_cross::CompilerMSL::Options::macOS;
        opts.set_msl_version(2, 1);
        opts.enable_decoration_binding = true;
        compiler.set_msl_options(opts);

        outMsl = compiler.compile();
        return true;
    } catch (const std::exception& e) {
        outError = e.what();
        return false;
    }
}

// ---------------------------------------------------------------------------
// Public: compile HLSL to MSL
// ---------------------------------------------------------------------------
MslResult compileHlslToMsl(
    const std::string& hlslFilePath,
    const std::string& hlslSource,
    const std::string& stage,
    const std::vector<std::string>& entryPoints,
    const std::vector<std::string>& includePaths,
    int ssboBindingShift)
{
    MslResult result;
    std::string combinedMsl;

    for (auto& entry : entryPoints) {
        std::vector<uint32_t> spirv;
        std::string error;

        if (!compileToSpirv(hlslFilePath, entry, stage, includePaths, ssboBindingShift, spirv, error)) {
            std::cerr << "[scribe-msl] glslang failed for entry '" << entry << "': " << error << std::endl;
            result.errorMessage = error;
            continue;
        }

        std::string msl;
        if (!convertToMsl(spirv, msl, error)) {
            std::cerr << "[scribe-msl] spirv-cross failed for entry '" << entry << "': " << error << std::endl;
            result.errorMessage = error;
            continue;
        }

        result.success = true;

        if (combinedMsl.empty()) {
            // First entry: keep the full MSL (structs, helpers, entry function)
            combinedMsl = msl;
        } else {
            // Subsequent entries: find the kernel/vertex/fragment function and append.
            // For compute shaders, spirv-cross preserves entry names (main_makeSkymap etc.)
            // so there's no duplicate 'main0' issue. For vert/frag, spirv-cross renames
            // all entries to 'main0' — skip subsequent entries to avoid redefinition.
            if (stage == "comp") {
                // For compute: each entry has a unique name (not renamed to main0).
                // Strip duplicate struct definitions but keep unique helpers
                // (spvUnsafeArray, spvTexelBufferCoord, etc.) that may not exist
                // in the first entry's MSL.
                combinedMsl += "\n// --- entry point: " + entry + " ---\n";

                // Process line by line: skip struct blocks, keep everything else
                std::istringstream stream(msl);
                std::string line;
                bool inStruct = false;
                int braceDepth = 0;
                while (std::getline(stream, line)) {
                    // Detect struct definitions (but not template struct like spvUnsafeArray)
                    if (!inStruct && line.find("struct ") == 0) {
                        // Check if this struct is already defined in combinedMsl
                        if (combinedMsl.find(line) != std::string::npos) {
                            inStruct = true;
                            braceDepth = 0;
                            for (char c : line) {
                                if (c == '{') braceDepth++;
                                if (c == '}') braceDepth--;
                            }
                            continue;
                        }
                    }
                    if (inStruct) {
                        for (char c : line) {
                            if (c == '{') braceDepth++;
                            if (c == '}') braceDepth--;
                        }
                        if (braceDepth <= 0) inStruct = false;
                        continue;
                    }
                    // Skip #pragma and #include that are duplicates
                    if (line.find("#pragma") == 0 || line.find("#include") == 0) {
                        if (combinedMsl.find(line) != std::string::npos) continue;
                    }
                    // Skip "using namespace metal;" duplicate
                    if (line.find("using namespace metal;") != std::string::npos) {
                        if (combinedMsl.find("using namespace metal;") != std::string::npos) continue;
                    }
                    combinedMsl += line + "\n";
                }
            }
            // For vert/frag: skip (spirv-cross renames all to main0 → redefinition)
        }
    }

    result.mslSource = combinedMsl;

    return result;
}

#endif // SCRIBE_HAS_MSL
