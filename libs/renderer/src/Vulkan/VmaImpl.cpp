// Single translation unit that compiles the Vulkan Memory Allocator
// implementation. We drive Vulkan through volk, so VMA must not statically or
// dynamically resolve its own entry points; the allocator is created with an
// explicit VmaVulkanFunctions table populated from volk (see VulkanAllocator).
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#include <volk.h>

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <vk_mem_alloc.h>

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
