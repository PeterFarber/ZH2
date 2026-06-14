# Vendored Dear ImGui Vulkan backend

`imgui_impl_vulkan.cpp` / `imgui_impl_vulkan.h` are copied verbatim from Dear
ImGui **v1.92.8 (docking)** — the same version provided by the vcpkg `imgui`
port used for the rest of the editor.

## Why vendored instead of using `imgui[vulkan-binding]`?

The renderer (`hockey_renderer`) uses **volk** with `VK_NO_PROTOTYPES`. volk
defines the Vulkan entry points (`vkCmdDrawIndexed`, `vkGetDeviceProcAddr`, …)
as **global function-pointer variables**. vcpkg builds ImGui's Vulkan backend
against real Vulkan prototypes, so its `vkCmd*` calls are direct calls to the
loader's functions. When both end up in one executable, the linker binds the
backend's function references to volk's *variables*, and ImGui ends up calling
into pointer storage as if it were code — an immediate crash.

Compiling this backend ourselves with `IMGUI_IMPL_VULKAN_USE_VOLK` makes it
include `volk.h` and route every Vulkan call through the *same* volk function
pointers the renderer already loads. That removes the conflict.

The `imgui` vcpkg dependency therefore enables only `docking-experimental` and
`sdl3-binding`; the Vulkan backend lives here.

## Updating

When the vcpkg `imgui` version changes, re-copy both files from the matching
`imgui/backends/` and keep this note's version in sync.
