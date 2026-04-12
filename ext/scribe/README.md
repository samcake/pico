# Scribe

Scribe is a build-time shader preprocessor and C++ code generator for the [pico](../../README.md) 3D framework. It takes HLSL shader source files, expands template directives, extracts resource metadata, and generates C++ header/source pairs that embed the shader source and expose its layout at compile time.

## Features

- **Template preprocessing** — `<@include>`, `<@if>`, `<@for>`, `<$var>` directives for shader code generation
- **C++ code generation** — Produces a class per shader with `getSource()`, `getShaderInit()`, resource layouts, and struct definitions
- **HLSL introspection** — Parses `cbuffer`, `ConstantBuffer<T>`, `StructuredBuffer<T>`, `Texture2D`, `SamplerState`, and `register()` declarations to extract descriptor layouts
- **MSL conversion** (macOS) — Converts HLSL to Metal Shading Language via glslangValidator + spirv-cross, embedding both in the same `.cpp` with `#ifdef __APPLE__` guards

## Usage

```
scribe <input.hlsl> [options] -o <output>
```

### Options

| Flag | Description |
|---|---|
| `-o <filename>` | Output file base name (generates `.h` and `.cpp`) |
| `-t <name>` | Set the target class name (default: derived from output filename) |
| `-c++` | Generate C++ header and source files |
| `-T <vert\|frag\|comp>` | Shader stage type (default: VERTEX) |
| `-parseFilenameType` | Deduce shader type from filename suffix (`_vert`, `_frag`, `_comp`, `_inc`) |
| `-msl` | Convert HLSL to MSL and embed both in the output (macOS only, requires spirv-cross + glslangValidator) |
| `-I <dir>` | Add an include search directory |
| `-D <name> <value>` | Define a template variable |
| `-H <file>` | Prepend a header file to the output |
| `-M` | Emit makefile dependency list |
| `-listVars` | Print all defined variables |
| `-showParseTree` | Print the parsed template tree |

### Examples

```bash
# Generate C++ wrapper for a vertex shader
scribe ModelPart_vert.hlsl -c++ -parseFilenameType -o shaders/ModelPart_vert

# Same, with MSL conversion for macOS
scribe ModelPart_vert.hlsl -c++ -parseFilenameType -msl -I render -I drawables -o shaders/ModelPart_vert
```

## Generated Output

For a shader `ModelPart_vert.hlsl`, scribe generates:

### `ModelPart_vert.h`
```cpp
class ModelPart_vert {
public:
    static const std::string& getSource();           // shader source (HLSL or MSL)
    static std::string getSourceFilename();           // original .hlsl path
    static std::string getName();                     // "ModelPart_vert"
    static const std::string& getType();              // "VERTEX"
    static const graphics::ShaderIncludes& getShaderIncludes(); // transitive includes
    static const graphics::DescriptorSetLayout& getPushLayout();     // push constants
    static const graphics::DescriptorSetLayout& getResourceLayout(); // bound resources
    static graphics::ShaderInit getShaderInit(const std::string& entry);

    // Mirrored HLSL structs (e.g. cbuffer contents)
    struct UniformBlock1 { ... };
};
```

### `ModelPart_vert.cpp`
```cpp
// Without -msl:
const std::string ModelPart_vert::_source = R"SCRIBE(... HLSL ...)SCRIBE";

// With -msl:
#ifndef __APPLE__
const std::string ModelPart_vert::_source = R"SCRIBE(... HLSL ...)SCRIBE";
#else
const std::string ModelPart_vert::_source = R"MSLSRC(... MSL ...)MSLSRC";
#endif
```

## MSL Conversion Pipeline

When `-msl` is specified (macOS builds with spirv-cross installed):

```
HLSL source
  │ glslangValidator CLI (subprocess)
  │   --shift-ssbo-binding <stage> 9
  │   --msl-decoration-binding
  ▼
SPIR-V binary
  │ spirv-cross C++ library (in-process)
  │   enable_decoration_binding = true
  │   msl_version = 2.1
  ▼
MSL source → embedded in .cpp with #ifdef __APPLE__
```

The SSBO binding shift (+9) separates `StructuredBuffer<T>` (`t` registers) from `cbuffer` (`b` registers) in Metal's unified buffer table. The Metal backend applies the same shift when binding descriptor sets.

## Build Requirements

**All platforms:**
- C++20 compiler
- No external dependencies (template engine is self-contained)

**macOS MSL conversion (optional):**
- `spirv-cross` — linked as a static library (`brew install spirv-cross`)
- `glslangValidator` — invoked as CLI tool (`brew install glslang`)

## Integration

In `src/graphics/CMakeLists.txt`, scribe is invoked per shader:

```cmake
add_custom_command(
    DEPENDS scribe
    MAIN_DEPENDENCY ${shader_hlsl}
    OUTPUT ${output}.h ${output}.cpp
    COMMAND scribe ${shader_hlsl} -c++ -parseFilenameType ${_msl_flag}
        -I ${render_dir} -I ${drawables_dir} -o ${output}
)
```
