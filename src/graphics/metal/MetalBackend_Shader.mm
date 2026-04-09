// MetalBackend_Shader.mm
// MSL shader compilation for pico Metal backend
//
// Sam Gateau / pico - 2024
// MIT License
#ifdef __APPLE__

#include "MetalBackend.h"
#include <iostream>
#include <cstdio>

using namespace graphics;

// ---------------------------------------------------------------------------
// createShader — compile MSL source at runtime via newLibraryWithSource:
// ---------------------------------------------------------------------------
ShaderPointer MetalBackend::createShader(const ShaderInit& init) {
    auto* sh = new MetalShaderBackend();
    sh->initShaderDesc(init);
    sh->_entryPoint = init.entryPoint.empty() ? "main0" : init.entryPoint;

    // Resolve source string
    std::string source;
    if (init.sourceGetter) {
        source = init.sourceGetter();
    } else if (!init.url.empty()) {
        FILE* f = fopen(init.url.c_str(), "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            source.resize((size_t)sz);
            fread(source.data(), 1, (size_t)sz, f);
            fclose(f);
        }
    }

    if (source.empty()) {
        std::cerr << "[Metal] Empty shader source for entry: " << sh->_entryPoint << "\n";
        return ShaderPointer(sh);
    }
    sh->_mslSource = source;

    // Compile MSL
    NSError* error = nil;
    MTLCompileOptions* opts = [[MTLCompileOptions alloc] init];
    opts.languageVersion = MTLLanguageVersion2_1;

    id<MTLLibrary> lib = [_device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                                               options:opts
                                                 error:&error];
    if (!lib) {
        std::cerr << "[Metal] Shader compile error for '" << sh->_entryPoint << "':\n"
                  << [[error localizedDescription] UTF8String] << "\n";
        return ShaderPointer(sh);
    }

    // Try the requested entry point, then "main0" (spirv-cross default)
    id<MTLFunction> fn = [lib newFunctionWithName:[NSString stringWithUTF8String:sh->_entryPoint.c_str()]];
    if (!fn && sh->_entryPoint != "main0") {
        fn = [lib newFunctionWithName:@"main0"];
        if (fn) sh->_entryPoint = "main0";
    }
    if (!fn) {
        std::cerr << "[Metal] Function '" << sh->_entryPoint << "' not found in library\n";
    }
    sh->_function = fn;

    return ShaderPointer(sh);
}

// ---------------------------------------------------------------------------
// createProgram — links a vertex + pixel (or compute) shader pair
// ---------------------------------------------------------------------------
ShaderPointer MetalBackend::createProgram(const ProgramInit& init) {
    auto* prog = new MetalShaderBackend();
    prog->initProgramDesc(init);
    ShaderInit sd;
    sd.type = (init.type == PipelineType::COMPUTE) ? ShaderType::COMPUTE : ShaderType::PROGRAM;
    prog->initShaderDesc(sd);
    return ShaderPointer(prog);
}

#endif // __APPLE__
