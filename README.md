# Pico
2020/01/01 This one is called 'Pico'

It is 2020, time to build yet another rendering engine and reinvent the wheel because why not. We want to build a simple 'engine' framework to easily explore the cool computer graphics ideas of the next decade.

Pico is built on a gpu api layer abstraction unifying the concepts of the modern graphics api (d3d12 / Vulkan / Metal). Haiming for simple, few objects with a clear mapping to their underneath equivalent and allowing to write interesting samples once for all the plaftorm s supported.

The scene management and rendering architecture mimics Apple SceneKit at a high level since it is well done & understood and easy to use (similar to Filament too).

The shader / kernel / program system is key to efficiently articulate algorithm as sequences of draw/compute passes. 
Any algorithm should be embeddable in a job executing on parameter data that can be chained with others.

This is the quest that we are after, design an elegant and powerful framework to achieve these goals.

## Samples & development
Each step of the way we will develop a new sample to explore and draft the next piece of feature until we we get to a stable version. THe library code is constantly evolving as we refine the design and we will retroactively apply the changes required to the previous samples to keep them all consistent.

###  pico 1: Clear a swapchain
introducing
- gpu::Device
- gpu::Swapchain
- window::Window
- render::Renderer

###  pico 2: Draw a triangle
introducing
- gpu::Shader
- gpu::Pipeline State 
- gpu::Buffer as Vertex & Index buffer
- gpu::StreamLayout
- gpu::RenderCallback

###  pico 3: Load & Draw a Pointcloud, change view/projection transform
introducing:
- gpu::Buffer as Uniform Buffer
- gpu::Descriptor and DescriptorSetLayout
- gpu::Buffer as Vertex & Index buffer
- document::Pointcloud
- render::Mesh

## Credits, Inspiration & References
- Hai Nguyen (chaoticbob) vulkan and D3d12 unifying abstraction https://github.com/chaoticbob/tinyrenderers
- Jeremiah van Oosten d3d12 tutorials https://www.3dgep.com/learning-directx-12-1/
- Apple SceneKit architecture & design https://developer.apple.com/scenekit/



