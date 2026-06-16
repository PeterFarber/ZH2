# Phase 3 Plan — Complete Vulkan Renderer Subsystem

File path recommendation:

```text
.cursor/plans/phase-03-vulkan-renderer.md
```

Cursor instruction:

```text
Read `.cursor/plans/phase-03-vulkan-renderer.md` and implement Phase 3 exactly. Build the complete shared Vulkan renderer subsystem for the game client and map editor. Do not implement the Unity-style editor UI, physics simulation, networking, GameNetworkingSockets, hockey gameplay simulation, audio, animation, or the final asset pipeline during this phase. Keep the dedicated server headless and make sure it does not link the renderer. Work step by step, update CMake whenever files are added, add tests/smoke tests, and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 3 builds the complete shared Vulkan renderer subsystem.

Used by:

```text
HockeyGameClient
HockeyMapEditor
```

Not used by:

```text
HockeyDedicatedServer
```

Phase 3 produces:

```text
hockey_renderer
HockeyRendererTests
```

Phase 3 updates:

```text
HockeyGameClient
HockeyMapEditor
hockey_ecs render-related component data
data/config/client.toml
data/config/editor.toml
data/shaders
```

Phase 3 must not implement:

```text
Unity-style editor panels
ImGui editor docking UI
physics simulation
Jolt
networking
GameNetworkingSockets
hockey gameplay simulation
audio
animation
final asset pipeline
```

The renderer should be complete enough that later phases can consume it without rewriting the architecture.


---

# 1. Architecture and Dependency Rules

## 1.1 Dependency Direction

Add:

```text
hockey_renderer -> hockey_core, hockey_ecs
```

Allowed app links:

```text
HockeyGameClient       -> hockey_core, hockey_ecs, hockey_renderer
HockeyMapEditor        -> hockey_core, hockey_ecs, hockey_renderer
HockeyRendererTests    -> hockey_core, hockey_ecs, hockey_renderer
```

Forbidden links:

```text
HockeyDedicatedServer -> hockey_renderer
HockeyDedicatedServer -> Vulkan
HockeyDedicatedServer -> Volk
HockeyDedicatedServer -> VMA
HockeyDedicatedServer -> ImGui
```

## 1.2 Renderer Owns

```text
Vulkan instance/device/swapchain
Volk initialization
VMA allocation
GPU resources
render graph
render targets
buffers
textures
samplers
descriptor management
pipeline management
shader compilation/loading
mesh rendering
material system
lighting
shadows
post-processing
debug rendering
offscreen viewport rendering
graphics settings
renderer stats/profiling hooks
```

## 1.3 Renderer Must Not Own

```text
hockey gameplay rules
network state
lobby state
physics simulation ownership
editor panels
scene serialization ownership
asset import/cooking pipeline
```

## 1.4 ECS/Renderer Boundary

ECS owns render component data:

```text
CameraComponent
MeshRendererComponent
LightComponent
EnvironmentComponent
ReflectionProbeComponent
DecalComponent
```

Renderer owns actual Vulkan objects and rendering behavior.

ECS components must not contain raw Vulkan handles:

```text
VkBuffer
VkImage
VkImageView
VkPipeline
VkDescriptorSet
VkCommandBuffer
VmaAllocation
```


---

# 2. Dependencies

Update `vcpkg.json`.

Add:

```json
"volk",
"vulkan-memory-allocator",
"vulkan-headers",
"shaderc",
"spirv-cross",
"spirv-tools",
"stb"
```

Optional only if implemented cleanly in this phase:

```json
"tracy"
```

Document required external tools:

```text
Vulkan SDK
RenderDoc
GPU vendor driver/tools
```

Do not add:

```text
GameNetworkingSockets
Jolt
Dear ImGui
ImGuizmo
FMOD/Wwise
full asset pipeline dependencies
```


---

# 3. Target Project Structure

Add:

```text
libs/
  renderer/
    CMakeLists.txt
    include/Hockey/Renderer/
      Renderer.hpp
      RendererInitInfo.hpp
      RendererSettings.hpp
      RendererStats.hpp
      RenderTypes.hpp
      RenderHandles.hpp
      RenderDevice.hpp
      RenderContext.hpp
      RenderGraph.hpp
      RenderPass.hpp
      RenderTarget.hpp
      FrameData.hpp
      Buffer.hpp
      Texture.hpp
      Sampler.hpp
      Mesh.hpp
      Material.hpp
      Shader.hpp
      Pipeline.hpp
      DescriptorSet.hpp
      Camera.hpp
      Light.hpp
      DebugDraw.hpp
      BuiltInResources.hpp
      Vulkan/
        VulkanCommon.hpp
        VulkanInstance.hpp
        VulkanDebug.hpp
        VulkanSurface.hpp
        VulkanDevice.hpp
        VulkanSwapchain.hpp
        VulkanAllocator.hpp
        VulkanCommand.hpp
        VulkanBuffer.hpp
        VulkanTexture.hpp
        VulkanSampler.hpp
        VulkanShader.hpp
        VulkanPipeline.hpp
        VulkanDescriptor.hpp
        VulkanFrameData.hpp
        VulkanRenderGraph.hpp

    src/
      Renderer.cpp
      RendererSettings.cpp
      RendererStats.cpp
      RenderGraph.cpp
      RenderPass.cpp
      RenderTarget.cpp
      Buffer.cpp
      Texture.cpp
      Sampler.cpp
      Mesh.cpp
      Material.cpp
      Shader.cpp
      Pipeline.cpp
      DescriptorSet.cpp
      Camera.cpp
      Light.cpp
      DebugDraw.cpp
      BuiltInResources.cpp
      Vulkan/
        VolkImpl.cpp
        VulkanInstance.cpp
        VulkanDebug.cpp
        VulkanSurface.cpp
        VulkanDevice.cpp
        VulkanSwapchain.cpp
        VulkanAllocator.cpp
        VulkanCommand.cpp
        VulkanBuffer.cpp
        VulkanTexture.cpp
        VulkanSampler.cpp
        VulkanShader.cpp
        VulkanPipeline.cpp
        VulkanDescriptor.cpp
        VulkanFrameData.cpp
        VulkanRenderGraph.cpp

apps/
  renderer_tests/
    CMakeLists.txt
    src/
      RendererTestsMain.cpp
      Test.hpp
      RendererSettingsTests.cpp
      ShaderCompileTests.cpp
      ResourceHandleTests.cpp
      RenderGraphTests.cpp
      HeadlessServerLinkTests.cpp

data/
  shaders/
    src/
      common.glsl
      mesh.vert
      pbr.frag
      depth_only.vert
      shadow.vert
      shadow.frag
      fullscreen_triangle.vert
      tonemap.frag
      bloom_downsample.frag
      bloom_upsample.frag
      debug_line.vert
      debug_line.frag
      debug_shape.vert
      debug_shape.frag
    bin/
```

Update ECS with render data components:

```text
libs/ecs/include/Hockey/ECS/RenderComponents.hpp
libs/ecs/src/RenderComponents.cpp
```


---

# 4. CMake Requirements

## 4.1 Root CMake

Add:

```cmake
add_subdirectory(libs/renderer)
```

When tests are enabled:

```cmake
add_subdirectory(apps/renderer_tests)
```

Ordering:

```text
libs/core
libs/ecs
libs/renderer
apps
```

## 4.2 Renderer Target

Create target:

```text
hockey_renderer
```

Required package discovery, adjusted to actual vcpkg imported target names:

```cmake
find_package(volk CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(shaderc CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(Stb REQUIRED)
```

Required links:

```text
hockey_core
hockey_ecs
volk::volk or matching vcpkg target
Vulkan::Headers or matching Vulkan headers target
VulkanMemoryAllocator target
shaderc target
SPIRV-Cross target
SPIRV-Tools target
stb include target if required
```

Do not link:

```text
hockey_gameplay
hockey_networking
hockey_physics
hockey_editor
```

## 4.3 App Links

Update:

```text
HockeyGameClient links hockey_renderer
HockeyMapEditor links hockey_renderer
HockeyRendererTests links hockey_renderer
```

Do not update `HockeyDedicatedServer` with renderer dependencies.

## 4.4 Server Link Guard

Add a test or CMake comment/check verifying:

```text
HockeyDedicatedServer does not link hockey_renderer.
```


---

# 5. Renderer Public API

## 5.1 `RendererInitInfo.hpp`

Implement:

```cpp
struct RendererInitInfo {
    Window* window = nullptr;
    RendererSettings settings;

    bool enableValidation = false;
    bool enableDebugMarkers = true;

    std::filesystem::path shaderSourceDirectory;
    std::filesystem::path shaderBinaryDirectory;
};
```

## 5.2 `Renderer.hpp`

Implement:

```cpp
class Renderer {
public:
    Renderer();
    ~Renderer();

    Status Init(const RendererInitInfo& info);
    void Shutdown();

    bool IsInitialized() const;

    void BeginFrame();
    void RenderScene(Scene& scene, const CameraRenderData& camera);
    void EndFrame();

    void Resize(uint32_t width, uint32_t height);

    TextureHandle CreateRenderTarget(const RenderTargetDesc& desc);
    void ResizeRenderTarget(TextureHandle target, uint32_t width, uint32_t height);
    void RenderSceneToTarget(Scene& scene, const CameraRenderData& camera, TextureHandle target);

    MeshHandle CreateMesh(const MeshDesc& desc);
    TextureHandle CreateTexture(const TextureDesc& desc);
    MaterialHandle CreateMaterial(const MaterialDesc& desc);

    MeshHandle GetBuiltInMesh(BuiltInMesh mesh) const;
    MaterialHandle GetBuiltInMaterial(BuiltInMaterial material) const;

    const RendererSettings& GetSettings() const;
    Status ApplySettings(const RendererSettings& settings);

    const RendererStats& GetStats() const;
};
```

Rules:

```text
No raw Vulkan handles in public gameplay/editor-facing APIs.
Use renderer handles and descriptions.
Only internal/debug methods may expose Vulkan handles, and they must be clearly named.
```

## 5.3 Render Handles

Implement typed handles:

```cpp
struct MeshHandle { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct TextureHandle { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct MaterialHandle { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct ShaderHandle { uint32_t id = 0; bool IsValid() const { return id != 0; } };
struct PipelineHandle { uint32_t id = 0; bool IsValid() const { return id != 0; } };
```

`id == 0` means invalid/null.


---

# 6. Renderer Settings and Stats

## 6.1 Renderer Settings

Implement complete `RendererSettings`.

Enums:

```cpp
enum class DisplayMode { Windowed, BorderlessFullscreen, ExclusiveFullscreen };
enum class GraphicsPreset { Low, Medium, High, Ultra, Custom };
enum class Upscaler { None, FSR, XeSS, DLSS };
enum class AntiAliasing { Off, FXAA, TAA, MSAA2x, MSAA4x, MSAA8x };
enum class TextureQuality { Low, Medium, High, Ultra };
enum class ShadowQuality { Off, Low, Medium, High, Ultra };
enum class AmbientOcclusionQuality { Off, Low, Medium, High, Ultra };
enum class ReflectionQuality { Off, Low, Medium, High, Ultra };
enum class EffectQuality { Off, Low, Medium, High, Ultra };
enum class DetailQuality { Low, Medium, High, Ultra };
enum class ToneMapper { Linear, Reinhard, ACES };
```

Struct:

```cpp
struct RendererSettings {
    DisplayMode displayMode = DisplayMode::Windowed;
    uint32_t resolutionWidth = 1920;
    uint32_t resolutionHeight = 1080;
    uint32_t refreshRate = 0;
    uint32_t monitorIndex = 0;
    bool vsync = true;
    uint32_t fpsLimit = 0;
    bool hdr = true;
    float brightness = 1.0f;
    float fieldOfView = 70.0f;

    float renderScale = 1.0f;
    bool dynamicResolution = false;
    Upscaler upscaler = Upscaler::None;
    float sharpening = 0.3f;

    GraphicsPreset preset = GraphicsPreset::High;

    TextureQuality textureQuality = TextureQuality::High;
    uint32_t textureStreamingBudgetMB = 4096;
    uint32_t anisotropy = 16;
    DetailQuality materialQuality = DetailQuality::High;

    ShadowQuality shadowQuality = ShadowQuality::High;
    float shadowDistance = 100.0f;
    bool contactShadows = true;
    AmbientOcclusionQuality aoQuality = AmbientOcclusionQuality::High;
    ReflectionQuality reflectionQuality = ReflectionQuality::High;
    DetailQuality globalIlluminationQuality = DetailQuality::Medium;

    DetailQuality modelQuality = DetailQuality::High;
    float lodDistanceMultiplier = 1.0f;
    DetailQuality crowdQuality = DetailQuality::Medium;
    DetailQuality arenaDetail = DetailQuality::High;
    DetailQuality boardGlassDetail = DetailQuality::High;

    bool bloom = true;
    bool motionBlur = false;
    bool depthOfField = false;
    bool lensFlare = true;
    DetailQuality volumetricLighting = DetailQuality::Medium;
    DetailQuality particleQuality = DetailQuality::High;

    DetailQuality iceQuality = DetailQuality::High;
    ReflectionQuality iceReflectionQuality = ReflectionQuality::High;
    DetailQuality iceScratchQuality = DetailQuality::High;
    EffectQuality skateSprayQuality = EffectQuality::High;
    EffectQuality puckTrailQuality = EffectQuality::Medium;
    DetailQuality jerseyQuality = DetailQuality::High;
    DetailQuality goalNetQuality = DetailQuality::High;

    AntiAliasing antiAliasing = AntiAliasing::FXAA;
    ToneMapper toneMapper = ToneMapper::ACES;
    bool filmGrain = false;
    bool chromaticAberration = false;
    bool vignette = false;

    bool showFPS = false;
    bool showFrameTime = false;
    bool showGPUStats = false;
    bool showNetworkStats = false;
};
```

Implement:

```cpp
RendererSettings MakeDefaultRendererSettings();
RendererSettings ApplyGraphicsPreset(GraphicsPreset preset, RendererSettings base = {});
Status LoadRendererSettings(Config& config, RendererSettings& outSettings);
void SaveRendererSettings(Config& config, const RendererSettings& settings);
```

Settings must affect:

```text
resolution
VSync
render scale
HDR target
field of view
anisotropy
shadow quality
AO quality
bloom enabled/disabled
tone mapper
debug stat flags
```

## 6.2 Renderer Stats

Implement:

```cpp
struct RendererStats {
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t meshCount = 0;
    uint32_t materialCount = 0;
    uint32_t textureCount = 0;

    float cpuFrameMs = 0.0f;
    float gpuFrameMs = 0.0f;

    uint64_t usedVRAMBytes = 0;
    uint64_t budgetVRAMBytes = 0;

    uint32_t shadowDrawCalls = 0;
    uint32_t postProcessPasses = 0;
};
```

Stats reset/update each frame.


---

# 7. Vulkan Base Implementation

## 7.1 Volk

Create:

```text
src/Vulkan/VolkImpl.cpp
```

Only this file defines:

```cpp
#define VOLK_IMPLEMENTATION
#include <volk.h>
```

All other Vulkan files:

```cpp
#include <volk.h>
```

Required calls:

```text
volkInitialize before Vulkan instance creation
volkLoadInstance after instance creation
volkLoadDevice after logical device creation
```

## 7.2 Vulkan Instance

Files:

```text
VulkanInstance.hpp/cpp
```

Responsibilities:

```text
create/destroy VkInstance
query supported layers/extensions
enable SDL-required surface extensions
enable VK_EXT_debug_utils in debug
log Vulkan API version
log enabled layers
log enabled extensions
```

## 7.3 Debug Messenger

Files:

```text
VulkanDebug.hpp/cpp
```

Implement:

```text
VK_EXT_debug_utils messenger
validation callback
debug object names
debug labels
```

Helpers:

```cpp
void SetObjectName(VkDevice device, uint64_t handle, VkObjectType type, const char* name);
void BeginDebugLabel(VkCommandBuffer cmd, const char* name);
void EndDebugLabel(VkCommandBuffer cmd);
```

Enable validation in debug:

```text
VK_LAYER_KHRONOS_validation
synchronization validation when feasible
```

## 7.4 SDL Vulkan Surface

Files:

```text
VulkanSurface.hpp/cpp
```

Responsibilities:

```text
create VkSurfaceKHR from SDL3 window
destroy surface
keep surface creation inside renderer
```


---

# 8. GPU, Device, VMA, Swapchain, Commands

## 8.1 GPU and Device

Files:

```text
VulkanDevice.hpp/cpp
RenderDevice.hpp
```

Implement:

```cpp
struct GPUInfo {
    std::string name;
    uint32_t vendorID = 0;
    uint32_t deviceID = 0;
    bool discrete = false;
    uint64_t dedicatedVideoMemory = 0;

    bool supportsTimelineSemaphore = false;
    bool supportsDescriptorIndexing = false;
    bool supportsSamplerAnisotropy = false;
    bool supportsFillModeNonSolid = false;
};
```

Require:

```text
graphics queue
present queue
swapchain support
samplerAnisotropy
```

Prefer:

```text
discrete GPU
compute queue
transfer queue
large VRAM
timeline semaphore
descriptor indexing
```

Log all candidate GPUs and chosen GPU.

## 8.2 VMA Allocator

Files:

```text
VulkanAllocator.hpp/cpp
```

Responsibilities:

```text
create/destroy VmaAllocator
create buffer allocations
create image allocations
track memory stats when available
```

## 8.3 Swapchain

Files:

```text
VulkanSwapchain.hpp/cpp
```

Implement:

```text
surface capability query
surface format selection
present mode selection
extent selection
swapchain creation
swapchain image view creation
swapchain recreation
old swapchain cleanup
resize handling
minimized-window handling
VSync on/off
sRGB output format when appropriate
```

Present modes:

```text
VSync on: VK_PRESENT_MODE_FIFO_KHR
VSync off: prefer MAILBOX, fallback IMMEDIATE, fallback FIFO
```

## 8.4 Frame Data

Files:

```text
FrameData.hpp
VulkanFrameData.hpp/cpp
```

Implement:

```cpp
struct FrameData {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    BufferHandle cameraBuffer;
    BufferHandle sceneBuffer;
};
```

Use 2 frames in flight by default.

## 8.5 Command System

Files:

```text
VulkanCommand.hpp/cpp
```

Implement:

```text
per-frame command pool
upload command pool
single-use command buffer helper
command buffer begin/end
queue submit
layout transition helpers
```

Helpers:

```cpp
VkCommandBuffer BeginSingleTimeCommands();
void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
```


---

# 9. GPU Resource Wrappers

## 9.1 Buffer

Files:

```text
Buffer.hpp/cpp
VulkanBuffer.hpp/cpp
```

Implement:

```cpp
enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage,
    Staging,
    Readback
};

struct BufferDesc {
    size_t size = 0;
    BufferUsage usage = BufferUsage::Vertex;
    bool cpuVisible = false;
    std::string debugName;
};
```

Required behavior:

```text
create buffer
destroy buffer
upload data
map/unmap CPU-visible buffer
copy buffer
debug name
```

## 9.2 Texture

Files:

```text
Texture.hpp/cpp
VulkanTexture.hpp/cpp
```

Implement:

```cpp
enum class TextureFormat {
    RGBA8,
    RGBA8_SRGB,
    RGBA16F,
    RGBA32F,
    Depth32F,
    Depth24Stencil8
};

enum class TextureUsage {
    Sampled,
    RenderTarget,
    DepthStencil,
    Storage,
    TransferSrc,
    TransferDst
};

struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::RGBA8;
    uint32_t usageFlags = 0;
    std::string debugName;
};
```

Required behavior:

```text
create image
allocate image with VMA
create image view
create sampler when needed
layout transitions
upload pixels
destroy texture
debug name
```

## 9.3 Sampler

Files:

```text
Sampler.hpp/cpp
VulkanSampler.hpp/cpp
```

Implement:

```cpp
enum class FilterMode {
    Nearest,
    Linear
};

enum class AddressMode {
    Repeat,
    ClampToEdge,
    ClampToBorder
};

struct SamplerDesc {
    FilterMode minFilter = FilterMode::Linear;
    FilterMode magFilter = FilterMode::Linear;
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    float maxAnisotropy = 16.0f;
};
```

Required samplers:

```text
default linear
default nearest
anisotropic
shadow sampler
```


---

# 10. Descriptors, Shaders, Pipelines

## 10.1 Descriptor System

Files:

```text
DescriptorSet.hpp/cpp
VulkanDescriptor.hpp/cpp
```

Implement descriptor management for:

```text
camera uniform buffer
scene/global uniform buffer
material data
sampled textures
shadow maps
post-process textures
```

Minimum design:

```text
global descriptor set
material descriptor set
per-pass descriptor set
```

Implement:

```text
descriptor pool
descriptor set layout
descriptor allocation
descriptor updates
cleanup
```

## 10.2 Shader System

Files:

```text
Shader.hpp/cpp
VulkanShader.hpp/cpp
```

Shader directories:

```text
data/shaders/src
data/shaders/bin
```

Required shaders:

```text
common.glsl
mesh.vert
pbr.frag
depth_only.vert
shadow.vert
shadow.frag
fullscreen_triangle.vert
tonemap.frag
bloom_downsample.frag
bloom_upsample.frag
debug_line.vert
debug_line.frag
debug_shape.vert
debug_shape.frag
```

Use shaderc:

```text
GLSL -> SPIR-V
compile error logging
write .spv output
load shader module
destroy shader module
```

API example:

```cpp
struct ShaderDesc {
    std::filesystem::path sourcePath;
    ShaderStage stage;
    std::string entryPoint = "main";
    std::vector<std::string> defines;
};
```

## 10.3 Pipeline System

Files:

```text
Pipeline.hpp/cpp
VulkanPipeline.hpp/cpp
```

Implement:

```text
graphics pipeline creation
pipeline layout creation
vertex input description
input assembly
rasterization state
depth/stencil state
blend state
multisampling state
dynamic viewport/scissor
pipeline destruction
debug names
```

Required pipelines:

```text
PBR opaque mesh pipeline
depth prepass pipeline
shadow pipeline
transparent mesh pipeline
fullscreen post-process pipeline
debug line pipeline
debug shape pipeline
```


---

# 11. Render Graph and Render Path

## 11.1 Render Graph

Files:

```text
RenderGraph.hpp/cpp
RenderPass.hpp/cpp
RenderTarget.hpp/cpp
VulkanRenderGraph.hpp/cpp
```

Required concepts:

```text
render pass node
pass name
read textures
write textures
depth attachment
color attachment
execution callback
resource lifetime tracking
resize handling
execution order
debug labels per pass
```

API example:

```cpp
class RenderGraph {
public:
    RenderPassHandle AddPass(const RenderPassDesc& desc);
    TextureHandle CreateTransientTexture(const TextureDesc& desc);

    Status Compile();
    void Execute(RenderContext& context);

    void Clear();
};
```

## 11.2 Required Functional Passes

Implement:

```text
DepthPrepass
ShadowPass
MainOpaquePass
Lighting/PBR path
TransparentPass
SSAOPass or AO pass
BloomDownsamplePass
BloomUpsamplePass
ToneMapPass
DebugDrawPass
PresentPass
```

## 11.3 Frame Flow

Implement:

```text
BeginFrame
  acquire swapchain image
  wait frame fence
  reset command pool
  begin command buffer
  update per-frame data

RenderScene
  gather camera
  gather lights
  gather mesh renderers
  build/execute render graph
  execute depth/shadow/main/post/debug passes

EndFrame
  transition/present swapchain
  submit command buffer
  present
  advance frame index
```

## 11.4 Required Targets

Implement:

```text
HDR color target
depth target
shadow map target
bloom targets
offscreen editor viewport target
swapchain present target
```

Recommended:

```text
HDR color: RGBA16F
Depth: Depth32F or supported equivalent
```


---

# 12. Camera, Mesh, Material

## 12.1 Camera

Files:

```text
Camera.hpp/cpp
```

Implement:

```cpp
struct CameraRenderData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 position;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
};
```

Helper:

```cpp
CameraRenderData BuildCameraRenderData(
    const TransformComponent& transform,
    const CameraComponent& camera,
    float aspectRatio
);
```

## 12.2 Mesh System

Files:

```text
Mesh.hpp/cpp
```

Vertex:

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 uv;
};
```

MeshDesc:

```cpp
struct MeshDesc {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string debugName;
};
```

Built-in mesh enum:

```cpp
enum class BuiltInMesh {
    Cube,
    Sphere,
    Capsule,
    Cylinder,
    Plane,
    PuckCylinder,
    PlayerCapsule,
    RinkPlane
};
```

Required built-ins:

```text
cube
sphere
capsule
cylinder
plane
debug puck cylinder
debug player capsule
debug rink plane
```

## 12.3 Material System

Files:

```text
Material.hpp/cpp
```

Implement:

```cpp
enum class AlphaMode {
    Opaque,
    Mask,
    Blend
};

struct MaterialDesc {
    glm::vec4 baseColor = {1, 1, 1, 1};

    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;

    glm::vec3 emissiveColor{0.0f};
    float emissiveStrength = 0.0f;

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;

    TextureHandle baseColorTexture;
    TextureHandle normalTexture;
    TextureHandle metallicRoughnessTexture;
    TextureHandle occlusionTexture;
    TextureHandle emissiveTexture;

    std::string debugName;
};
```

Built-in material enum:

```cpp
enum class BuiltInMaterial {
    DefaultWhite,
    ErrorMagenta,
    Ice,
    PuckBlack,
    HomeJersey,
    AwayJersey,
    Boards,
    Glass,
    GoalNet,
    DebugCollider,
    DebugTrigger
};
```

Required built-ins:

```text
default white
error magenta
ice
puck black
home jersey
away jersey
boards
glass
goal net
debug collider
debug trigger
```


---

# 13. ECS Render Components

Add ECS render components and serialize/register metadata.

Preferred files:

```text
libs/ecs/include/Hockey/ECS/RenderComponents.hpp
libs/ecs/src/RenderComponents.cpp
```

Implement:

```cpp
struct CameraComponent {
    float fovDegrees = 70.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    bool primary = false;
};

struct MeshRendererComponent {
    std::string meshName;
    std::string materialName;
    bool visible = true;
    bool castsShadows = true;
    bool receivesShadows = true;
};

struct LightComponent {
    enum class Type {
        Directional,
        Point,
        Spot
    };

    Type type = Type::Directional;
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeDegrees = 15.0f;
    float outerConeDegrees = 30.0f;
    bool castsShadows = true;
};

struct EnvironmentComponent {
    glm::vec3 ambientColor{0.03f};
    float ambientIntensity = 1.0f;
};

struct ReflectionProbeComponent {
    float radius = 10.0f;
    float intensity = 1.0f;
};

struct DecalComponent {
    std::string materialName;
    glm::vec3 size{1.0f};
    bool affectsBaseColor = true;
    bool affectsNormals = true;
};
```

Update:

```text
ComponentSerializer
ComponentRegistry
SceneSerializer
SceneValidator if needed
```

Because final asset pipeline does not exist yet, resource references use strings:

```text
meshName = "BuiltIn.RinkPlane"
materialName = "BuiltIn.Ice"
```


---

# 14. Lighting, Shadows, AO, Post, Transparency

## 14.1 Lighting

Implement:

```text
directional light
point light
spot light
ambient/environment light
```

## 14.2 Shadows

Implement:

```text
directional shadow map
cascaded directional shadows
shadow quality settings
shadow distance settings
shadow draw stats
```

Minimum complete requirement:

```text
directional shadows working
CascadedShadowMap path working
shadow quality changes resolution/cascade count
```

Suggested mapping:

```text
Off: no shadow pass
Low: 1024, 2 cascades
Medium: 2048, 3 cascades
High: 4096, 4 cascades
Ultra: 4096/8192, 4 cascades, improved filtering
```

## 14.3 Ambient Occlusion

Implement AO or SSAO:

```text
can be disabled
controlled by AmbientOcclusionQuality
composited into lighting/post process
```

## 14.4 Bloom

Implement:

```text
threshold/extract
downsample chain
upsample/combine
enable/disable setting
quality control
```

## 14.5 Tone Mapping

Implement:

```text
Linear
Reinhard
ACES
```

Settings:

```text
brightness
toneMapper
HDR
```

## 14.6 Anti-Aliasing

Implement at least one complete path:

```text
Off
FXAA
```

If TAA is implemented, include:

```text
jitter
history target
resolve
resize history reset
```

Do not claim TAA works unless it actually works.

## 14.7 Transparency

Implement:

```text
transparent render queue
back-to-front sorting
blend pipeline
glass material support
```

Needed for rink glass.


---

# 15. Debug Draw and Offscreen Viewport

## 15.1 DebugDraw

Files:

```text
DebugDraw.hpp/cpp
```

API:

```cpp
class DebugDraw {
public:
    void DrawLine(glm::vec3 a, glm::vec3 b, glm::vec4 color);
    void DrawBox(glm::mat4 transform, glm::vec4 color);
    void DrawSphere(glm::vec3 center, float radius, glm::vec4 color);
    void DrawCapsule(glm::vec3 center, float height, float radius, glm::vec4 color);
    void Clear();
};
```

Required:

```text
debug lines render
debug boxes render
debug spheres/capsules render or approximated
debug draw pass after main scene
debug shapes do not affect normal scene
```

## 15.2 Offscreen Editor Viewport

Phase 4 builds editor UI, but Phase 3 must provide viewport render target support.

Implement:

```cpp
struct RenderTargetDesc {
    uint32_t width = 1280;
    uint32_t height = 720;
    TextureFormat colorFormat = TextureFormat::RGBA16F;
    bool hasDepth = true;
    std::string debugName;
};
```

Required API:

```cpp
TextureHandle CreateRenderTarget(const RenderTargetDesc& desc);
void ResizeRenderTarget(TextureHandle target, uint32_t width, uint32_t height);
void RenderSceneToTarget(Scene& scene, const CameraRenderData& camera, TextureHandle target);
```

Requirements:

```text
map editor can render scene into offscreen texture
offscreen target resizes
offscreen target can later be sampled/displayed by ImGui
offscreen path shares renderer architecture
```


---

# 16. Built-In Scene Rendering and Hockey Visual Support

## 16.1 Built-In Mesh Names

Support:

```text
BuiltIn.Cube
BuiltIn.Sphere
BuiltIn.Capsule
BuiltIn.Cylinder
BuiltIn.Plane
BuiltIn.PuckCylinder
BuiltIn.PlayerCapsule
BuiltIn.RinkPlane
```

## 16.2 Built-In Material Names

Support:

```text
BuiltIn.DefaultWhite
BuiltIn.ErrorMagenta
BuiltIn.Ice
BuiltIn.PuckBlack
BuiltIn.HomeJersey
BuiltIn.AwayJersey
BuiltIn.Boards
BuiltIn.Glass
BuiltIn.GoalNet
BuiltIn.DebugCollider
BuiltIn.DebugTrigger
```

## 16.3 Scene Rendering

Renderer should:

```text
iterate active entities with TransformComponent and MeshRendererComponent
resolve meshName to built-in mesh
resolve materialName to built-in material
render visible mesh renderers
use CameraComponent primary camera where requested
gather LightComponent entities
render debug fallback if mesh/material missing
```

## 16.4 Hockey Visual Support

Implement renderer support/data paths for:

```text
ice material
ice roughness variation
ice reflection quality setting
ice scratch quality setting/data path
board/glass material
goal net material
puck material
team jersey materials
skate spray quality setting/data path
puck trail quality setting/data path
```

Do not implement hockey gameplay.


---

# 17. Config Integration

Update `client.toml` and `editor.toml` with:

```toml
[renderer]
display_mode = "Windowed"
resolution_width = 1920
resolution_height = 1080
refresh_rate = 0
monitor_index = 0
vsync = true
fps_limit = 0
hdr = true
brightness = 1.0
field_of_view = 70.0
render_scale = 1.0
dynamic_resolution = false
upscaler = "None"
sharpening = 0.3
preset = "High"
texture_quality = "High"
texture_streaming_budget_mb = 4096
anisotropy = 16
material_quality = "High"
shadow_quality = "High"
shadow_distance = 100.0
contact_shadows = true
ao_quality = "High"
reflection_quality = "High"
global_illumination_quality = "Medium"
model_quality = "High"
lod_distance_multiplier = 1.0
crowd_quality = "Medium"
arena_detail = "High"
board_glass_detail = "High"
bloom = true
motion_blur = false
depth_of_field = false
lens_flare = true
volumetric_lighting = "Medium"
particle_quality = "High"
ice_quality = "High"
ice_reflection_quality = "High"
ice_scratch_quality = "High"
skate_spray_quality = "High"
puck_trail_quality = "Medium"
jersey_quality = "High"
goal_net_quality = "High"
anti_aliasing = "FXAA"
tone_mapper = "ACES"
film_grain = false
chromatic_aberration = false
vignette = false
show_fps = false
show_frame_time = false
show_gpu_stats = false
show_network_stats = false
```

No renderer section is required for `server.toml`.


---

# 18. App Integration

## 18.1 Game Client

Update `HockeyGameClient`.

Required:

```text
initialize renderer after window creation
load renderer settings
load Phase 2 scene
ensure scene has camera or create default camera entity
ensure scene has light/environment or create defaults
render scene to swapchain
handle resize
apply FPS cap from settings/app layer
log renderer stats if enabled
shutdown renderer before window destruction
```

No gameplay or networking.

## 18.2 Map Editor

Update `HockeyMapEditor`.

Required:

```text
initialize renderer after window creation
load renderer settings
load Phase 2 scene
create editor camera if needed
create offscreen viewport render target
render scene to offscreen target
temporarily blit/copy offscreen target to swapchain until ImGui UI exists
handle window/viewport resize
shutdown renderer cleanly
```

No ImGui panels.

## 18.3 Dedicated Server

Verify:

```text
does not link hockey_renderer
does not initialize Vulkan
does not load renderer settings
still loads scene headlessly
still runs fixed tick
```


---

# 19. Renderer Tests

Create:

```text
HockeyRendererTests
```

Use simple test harness.

Classify tests as:

```text
pure unit tests
GPU smoke tests
manual validation tests
```

## 19.1 RendererSettingsTests

Test:

```text
default settings valid
preset Low changes expected fields
preset Medium changes expected fields
preset High changes expected fields
preset Ultra changes expected fields
settings load from config
settings save to config
enum string conversions work
```

## 19.2 ShaderCompileTests

Test:

```text
all required shaders exist
all required shaders compile to SPIR-V
compile failure returns useful error
compiled output path exists
```

## 19.3 ResourceHandleTests

Test:

```text
invalid handle id 0
created mesh handle valid
created material handle valid
created texture handle valid
built-in handles valid
```

## 19.4 RenderGraphTests

Test:

```text
can add pass
can add resource
compile orders passes
resize invalidates/recreates resources
missing dependency reports error
```

## 19.5 RendererSmokeTests

When GPU/display available:

```text
create SDL window
initialize renderer
render one frame
resize window
render another frame
shutdown renderer
no validation errors
```

## 19.6 HeadlessServerLinkTests

Verify or document:

```text
HockeyDedicatedServer does not link hockey_renderer
HockeyDedicatedServer does not include renderer headers
```


---

# 20. Debug, Validation, and RenderDoc Requirements

Phase 3 is not complete until:

```text
Vulkan validation layers work in debug
debug messenger logs warnings/errors
debug names assigned to important Vulkan objects
debug labels around render passes
RenderDoc can capture a frame
swapchain resize works
minimize/restore works
renderer shutdown has no validation errors
```

Name important objects:

```text
instance
device
swapchain images
frame command buffers
HDR color target
depth target
shadow maps
pipelines
buffers
textures
descriptor sets/layouts
render passes
```


---

# 21. Implementation Order for Cursor

Implement in this exact order.

## Step 1 — Dependency and Target Setup

```text
1. Add renderer dependencies to vcpkg.json.
2. Create libs/renderer folder.
3. Create libs/renderer/CMakeLists.txt.
4. Add hockey_renderer target.
5. Create apps/renderer_tests.
6. Add HockeyRendererTests target.
7. Update root CMakeLists.txt.
8. Link client/editor/renderer_tests to hockey_renderer.
9. Confirm server does not link hockey_renderer.
10. Configure/build before complex code.
```

## Step 2 — Renderer Types and Settings

```text
1. Add RenderTypes.hpp.
2. Add RenderHandles.hpp.
3. Add RendererSettings.hpp/cpp.
4. Add RendererStats.hpp/cpp.
5. Add RendererInitInfo.hpp.
6. Add enum string conversions.
7. Add config load/save for renderer settings.
8. Add RendererSettingsTests.
```

## Step 3 — Vulkan Base

```text
1. Add VulkanCommon.hpp.
2. Add VolkImpl.cpp.
3. Add VulkanInstance.hpp/cpp.
4. Add VulkanDebug.hpp/cpp.
5. Add VulkanSurface.hpp/cpp.
6. Add VulkanDevice.hpp/cpp.
7. Add VulkanAllocator.hpp/cpp.
8. Verify instance/device/allocation init and shutdown.
```

## Step 4 — Swapchain and Frame Data

```text
1. Add VulkanSwapchain.hpp/cpp.
2. Add FrameData.hpp.
3. Add VulkanFrameData.hpp/cpp.
4. Add VulkanCommand.hpp/cpp.
5. Implement BeginFrame/EndFrame basic flow.
6. Clear swapchain image.
7. Handle resize.
8. Handle minimized window.
9. Smoke test renderer init/frame/shutdown.
```

## Step 5 — Resource Wrappers

```text
1. Add Buffer.hpp/cpp.
2. Add VulkanBuffer.hpp/cpp.
3. Add Texture.hpp/cpp.
4. Add VulkanTexture.hpp/cpp.
5. Add Sampler.hpp/cpp.
6. Add VulkanSampler.hpp/cpp.
7. Implement upload helpers.
8. Add resource handle tests.
```

## Step 6 — Shaders and Pipelines

```text
1. Add Shader.hpp/cpp.
2. Add VulkanShader.hpp/cpp.
3. Add shader source files.
4. Implement shaderc compilation.
5. Add shader compile tests.
6. Add Pipeline.hpp/cpp.
7. Add VulkanPipeline.hpp/cpp.
8. Create required pipelines.
```

## Step 7 — Descriptors

```text
1. Add DescriptorSet.hpp/cpp.
2. Add VulkanDescriptor.hpp/cpp.
3. Implement global descriptor set.
4. Implement material descriptor set.
5. Implement per-pass descriptor resources.
6. Bind camera/scene/material data.
```

## Step 8 — Meshes and Materials

```text
1. Add Mesh.hpp/cpp.
2. Implement Vertex format.
3. Implement mesh upload.
4. Generate built-in meshes.
5. Add Material.hpp/cpp.
6. Implement MaterialDesc.
7. Implement built-in materials.
8. Add material/mesh handle tests.
```

## Step 9 — ECS Render Components

```text
1. Add RenderComponents.hpp/cpp in ECS.
2. Add CameraComponent.
3. Add MeshRendererComponent.
4. Add LightComponent.
5. Add EnvironmentComponent.
6. Add ReflectionProbeComponent.
7. Add DecalComponent.
8. Update serialization.
9. Update metadata registry.
10. Update scene tests as needed.
```

## Step 10 — Render Graph

```text
1. Add RenderGraph.hpp/cpp.
2. Add RenderPass.hpp/cpp.
3. Add RenderTarget.hpp/cpp.
4. Add VulkanRenderGraph.hpp/cpp.
5. Implement pass creation.
6. Implement resource creation.
7. Implement compile.
8. Implement execute.
9. Add RenderGraphTests.
```

## Step 11 — Scene Rendering

```text
1. Add Camera.hpp/cpp.
2. Add Light.hpp/cpp.
3. Gather cameras from scene.
4. Gather lights from scene.
5. Gather mesh renderers from scene.
6. Resolve built-in meshes/materials.
7. Render opaque meshes.
8. Render transparent meshes.
9. Render debug fallback for missing mesh/material.
```

## Step 12 — Shadows, AO, Post

```text
1. Implement depth prepass.
2. Implement shadow pass.
3. Implement cascaded directional shadows.
4. Implement AO pass.
5. Implement bloom passes.
6. Implement tone mapping.
7. Implement FXAA or chosen anti-aliasing path.
8. Wire settings to quality levels.
```

## Step 13 — Debug Draw and Viewport Targets

```text
1. Add DebugDraw.hpp/cpp.
2. Implement line drawing.
3. Implement box drawing.
4. Implement sphere/capsule approximations.
5. Add debug draw pass.
6. Implement CreateRenderTarget.
7. Implement ResizeRenderTarget.
8. Implement RenderSceneToTarget.
9. Verify offscreen editor rendering.
```

## Step 14 — App Integration

```text
1. Update client.toml renderer section.
2. Update editor.toml renderer section.
3. Update GameClientApp renderer lifecycle.
4. Add default camera/light if missing.
5. Render scene in client.
6. Update MapEditorApp renderer lifecycle.
7. Render scene to offscreen target in editor.
8. Blit/present editor target to swapchain.
9. Confirm server unchanged/headless.
```

## Step 15 — Final Verification

```text
1. Build Debug Windows.
2. Build Release Windows.
3. Build Debug Linux.
4. Build Release Linux.
5. Run HockeyCoreTests.
6. Run HockeyECSTests.
7. Run HockeyRendererTests.
8. Run client.
9. Run editor.
10. Run server.
11. Capture frame in RenderDoc.
12. Resize/minimize windows.
13. Confirm no validation errors.
14. Confirm no server renderer dependency.
```


---

# 22. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\build\windows-debug\apps\core_tests\HockeyCoreTests.exe --root .
.\build\windows-debug\apps\ecs_tests\HockeyECSTests.exe --root .
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root .
.\scripts\windows\run_client.ps1
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_server.ps1
```

## Linux

```bash
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./build/linux-debug/apps/core_tests/HockeyCoreTests --root .
./build/linux-debug/apps/ecs_tests/HockeyECSTests --root .
./build/linux-debug/apps/renderer_tests/HockeyRendererTests --root .
./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh
```


---

# 23. Phase 3 Definition of Done

Phase 3 is complete only when all of this is true:

```text
renderer dependencies added through vcpkg
hockey_renderer builds
HockeyRendererTests builds
HockeyRendererTests pass where environment supports GPU tests
HockeyGameClient links hockey_renderer
HockeyMapEditor links hockey_renderer
HockeyDedicatedServer does not link hockey_renderer
Volk initializes correctly
Vulkan instance creates
Vulkan debug messenger works
SDL Vulkan surface creates
physical device selection works
logical device creation works
VMA allocator works
swapchain works
BeginFrame/EndFrame work
window resize works
minimize/restore does not crash
frame synchronization works
buffer wrapper works
texture wrapper works
sampler wrapper works
descriptor system works
shader compilation works
required shaders compile
pipeline system works
mesh system works
built-in meshes work
material system works
built-in materials work
ECS render components exist
ECS render components serialize
ECS render components have metadata
render graph works
HDR color target works
depth target works
shadow map target works
directional lighting works
directional shadows work
AO pass works or is quality-controlled
bloom works
tone mapping works
anti-aliasing path works
transparent rendering works
debug drawing works
offscreen editor viewport target works
client renders scene to swapchain
editor renders scene to offscreen target and presents it
renderer settings model is complete
renderer settings load from config
renderer settings affect implemented systems
renderer stats update
RenderDoc can capture a frame
normal startup/render/shutdown has no Vulkan validation errors
server remains headless
no physics simulation exists
no networking code exists
no hockey gameplay simulation exists
no ImGui editor UI exists
```

---

# 24. Cursor Completion Instruction

When Phase 3 is complete, Cursor should report:

```text
Phase 3 complete.

Implemented:
- hockey_renderer
- HockeyRendererTests
- Volk/Vulkan instance/debug/surface/device
- VMA allocator
- swapchain/frame synchronization
- command system
- buffers/textures/samplers
- descriptor system
- shader compiler and required shaders
- pipeline system
- mesh system and built-in meshes
- material system and built-in materials
- ECS render components + serialization + metadata
- render graph
- HDR/depth/shadow/offscreen targets
- scene rendering
- lighting and shadows
- AO/bloom/tone mapping/anti-aliasing path
- transparent rendering
- debug drawing
- renderer settings/stats
- client renderer integration
- editor offscreen renderer integration

Verified:
- renderer tests
- client renders
- editor renders
- server remains headless
- resize/minimize handling
- RenderDoc capture
- Vulkan validation clean
- no future-phase systems were implemented
```
