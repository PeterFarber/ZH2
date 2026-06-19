# Phase 3 - Vulkan Renderer

Status: Implemented, with known renderer gaps.

Source material:

- [x] Read `docs/phase-plans/phase-03-vulkan-renderer.md`.
- [x] Read `docs/phase-rules/070-phase-3-vulkan-renderer.mdc`.
- [x] Checked `docs/project-structure.md`.
- [x] Checked `README.md` known-gap notes.

## What This Phase Implements

- [x] `hockey_renderer` Vulkan renderer library.
- [x] Renderer public API and opaque resource handles.
- [x] Vulkan base stack, GPU resources, descriptors, shaders, pipelines, and frame synchronization.
- [x] ECS-driven scene rendering.
- [x] Lighting, shadows, AO, bloom, tone mapping, anti-aliasing path, transparency, debug draw, and offscreen editor viewports.
- [x] Renderer settings/stats, renderer tests, client integration, editor integration, and headless server guard.

## Finished - Target And Boundaries

- [x] `hockey_renderer` target exists.
- [x] `HockeyRendererTests` target exists.
- [x] Renderer depends on core and ECS and privately consumes renderer-owned Vulkan dependencies.
- [x] Renderer privately owns Vulkan/volk/VMA/shaderc/stb image write details.
- [x] Dedicated server remains headless and does not link renderer, Vulkan, editor, or ImGui.
- [x] Renderer does not own physics, gameplay rules, networking, or editor UI.

## Finished - Public API, Settings, And Stats

- [x] Renderer initialization info exists.
- [x] Renderer pimpl API exists.
- [x] Opaque handles exist for renderer resources.
- [x] Renderer settings load/save from TOML.
- [x] Renderer stats include CPU frame timing.
- [x] Material preview support exists for editor UI.
- [x] Built-in screenshot request/readback API exists.

## Finished - Vulkan Base And Resources

- [x] Volk loader integration exists.
- [x] Vulkan instance creation exists.
- [x] Vulkan validation/debug messenger support exists.
- [x] SDL Vulkan surface creation exists.
- [x] GPU/device selection exists.
- [x] VMA allocator exists.
- [x] Swapchain creation/recreation exists.
- [x] Frame data and synchronization exist.
- [x] Command pool/buffer submission path exists.
- [x] Buffer wrapper exists.
- [x] Texture wrapper exists.
- [x] Sampler wrapper exists.
- [x] Descriptor system exists.
- [x] Shader system and shader compilation path exist.
- [x] Pipeline system exists.

## Finished - Render Path

- [x] Built-in meshes exist.
- [x] Built-in hockey materials exist.
- [x] Asset-backed mesh/material/texture upload and caching exist.
- [x] ECS camera, mesh renderer, light, and environment data are consumed at render time.
- [x] Forward PBR scene rendering exists.
- [x] Directional cascaded shadow rendering exists.
- [x] Spot/point local-light shadow atlas exists.
- [x] Depth prepass exists.
- [x] SSAO and blur exist.
- [x] Bloom downsample/upsample/composite exists.
- [x] Tone mapping exists.
- [x] FXAA exists.
- [x] Transparency path exists.
- [x] Debug line and debug shape drawing exists.
- [x] Offscreen render targets exist for editor Scene/Game viewports.
- [x] Swapchain and render-target PNG readback exists.
- [x] `RenderGraph` abstraction and tests exist.

## Finished - Tests And Verification

- [x] Renderer settings tests exist.
- [x] Shader compile tests exist.
- [x] Resource handle tests exist.
- [x] Render graph tests exist.
- [x] Renderer smoke paths exist.
- [x] Headless server link invariant tests exist.
- [x] Client renderer integration exists.
- [x] Editor offscreen renderer integration exists.
- [x] Screenshot workflow exists through app flags.

## Started / Partial

- [ ] Production frame sequence is still hard-coded in `Renderer.cpp`; `RenderGraph` is not the production execution path.
- [ ] TAA/MSAA settings exist, but only FXAA is implemented.
- [ ] `MeshRendererComponent::receivesShadows` is not fully honored by runtime shadow sampling.
- [ ] `contactShadows` is not fully wired as a renderer effect.
- [ ] Reflection probe components serialize, but renderer probe capture/sampling is not implemented.
- [ ] Decal components serialize, but decal projection rendering is not implemented.
- [ ] Some renderer settings are persisted only and are not live-applied.
- [ ] Hockey-specific polish visuals such as ice reflections, skate spray, and puck trails are material/settings names only.
- [ ] `gpuFrameMs` is not populated.
- [ ] Tracy or equivalent profiling integration is not implemented.
- [ ] Pixel-correct rendering, minimize behavior, and shadow atlas correctness still need display/GPU verification.

## Left To Do

- [ ] Route production rendering through `RenderGraph` if a renderer refactor is requested.
- [ ] Add TAA motion vectors/history buffers if TAA is requested.
- [ ] Add MSAA render-pass/pipeline changes if MSAA is requested.
- [ ] Implement reflection probes and decals when renderer polish resumes.
- [ ] Implement live sampler/descriptor recreation for relevant setting changes.
- [ ] Add GPU timestamp queries and profiling integration.
- [ ] Add Phase 9 visual polish systems for ice, skates, puck trails, crowd, and presentation.
- [ ] Run visual verification on a real GPU/display before closing renderer-only visual gaps.
