# Pico
2020/01/01 This one is called 'Pico'

It is 2020, time to build yet another rendering engine and reinvent the wheel because why not. We want to build a simple 'engine' framework to easily explore the cool computer graphics ideas of the next decade.

Pico is built on a gpu api layer abstraction unifying the concepts of the modern graphics api (d3d12 / Vulkan / Metal). Aiming for simple, few objects with a clear mapping to their underneath equivalent and allowing to write interesting samples once for all the plaftorm(s) supported.

The scene management and rendering architecture mimics Apple SceneKit at a high level since it is well done & understood and easy to use (similar to Filament too).

The shader / kernel / program system is key to efficiently articulate algorithm as sequences of draw/compute passes. 
Any algorithm should be embeddable in a job executing on parameter data that can be chained with others.

This is the quest that we are after, design an elegant and powerful framework to achieve these goals.

## Samples & development
Each step of the way we will develop a new sample to explore and draft the next piece of feature until we we get to a stable version. The library code is constantly evolving as we refine the design and we will retroactively apply the changes required to the previous samples to keep them all consistent.

### pico 1: Clear a swapchain
introducing
- gpu::Device
- gpu::Swapchain
- uix::Window
- render::Renderer

### pico 2: Draw a triangle
introducing
- gpu::Shader
- gpu::Pipeline State 
- gpu::Buffer as Vertex & Index buffer
- gpu::StreamLayout
- gpu::RenderCallback

### pico 3: Load & Draw a Pointcloud, change view/projection transform
introducing:
- gpu::Buffer as Uniform Buffer
- gpu::Descriptor and DescriptorSetLayout
- gpu::Buffer as Vertex & Index buffer
- document::Pointcloud
- render::Mesh

### pico 4: Scene, Viewport and Camera to render a simple 3d scene
introducing:
- render::Scene
- render::Camera
- render::Viewport
- drawable::PointCloudDrawable
- drawable::TriangleSoupDrawable

## Credits, Inspiration & References
- Hai Nguyen (chaoticbob) vulkan and D3d12 unifying abstraction https://github.com/chaoticbob/tinyrenderers
- Jeremiah van Oosten d3d12 tutorials https://www.3dgep.com/learning-directx-12-1/
- Apple SceneKit architecture & design https://developer.apple.com/scenekit/


## Build

The project uses **CMakePresets.json** for cross-platform, IDE-independent build configuration. All presets are defined there — no manual cmake flags needed.

### Requirements

- CMake 3.19+
- **Windows:** Visual Studio 2022 (MSVC), DirectX 12 SDK
- **macOS:** Xcode + Ninja (`brew install ninja`), Metal

### Configure & Build

**Windows (Visual Studio 2022):**
```bash
cmake --preset windows-release
cmake --build --preset windows-release-all
```

**macOS (Ninja):**
```bash
cmake --preset macos-debug
cmake --build --preset macos-debug-all
```

To build a single sample (e.g. `pico_04`):
```bash
cmake --build --preset windows-debug-pico_04
```

Binaries are output to `build/bin/Debug/` or `build/bin/Release/` on Windows, and `build_mac/bin/` on macOS.

### IDE support

- **Visual Studio 2022:** open the root folder — VS reads `CMakePresets.json` natively and sets up all targets automatically.
- **Zed:** build and run tasks are defined in `.zed/tasks.json` (gitignored, Windows-local). Debugger configs are in `.zed/debug.json`.

The CI workflow builds all targets on Windows using the `windows-release-all` preset — see [.github/workflows/main.yml](.github/workflows/main.yml).
