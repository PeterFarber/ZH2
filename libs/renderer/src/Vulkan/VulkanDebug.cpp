#include "Hockey/Renderer/Vulkan/VulkanDebug.hpp"

namespace Hockey {

namespace {

bool g_DebugUtilsEnabled = false;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT* data, void* /*user*/) {
    if (data == nullptr || data->pMessage == nullptr) {
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        HK_CORE_ERROR("[Vulkan] {}", data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        HK_CORE_WARN("[Vulkan] {}", data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        HK_CORE_INFO("[Vulkan] {}", data->pMessage);
    } else {
        HK_CORE_TRACE("[Vulkan] {}", data->pMessage);
    }
    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT MakeMessengerInfo() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = DebugCallback;
    return info;
}

} // namespace

Status VulkanDebug::Create(VkInstance instance) {
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        HK_CORE_WARN("VK_EXT_debug_utils not available; debug messenger disabled");
        return Status::Ok();
    }
    VkDebugUtilsMessengerCreateInfoEXT info = MakeMessengerInfo();
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &info, nullptr, &m_Messenger));
    g_DebugUtilsEnabled = true;
    HK_CORE_INFO("Vulkan debug messenger created");
    return Status::Ok();
}

void VulkanDebug::Destroy(VkInstance instance) {
    if (m_Messenger != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT != nullptr) {
        vkDestroyDebugUtilsMessengerEXT(instance, m_Messenger, nullptr);
        m_Messenger = VK_NULL_HANDLE;
    }
    g_DebugUtilsEnabled = false;
}

bool VulkanDebug::IsAvailable() {
    return g_DebugUtilsEnabled;
}

void SetObjectName(VkDevice device, uint64_t handle, VkObjectType type, const char* name) {
    if (!g_DebugUtilsEnabled || vkSetDebugUtilsObjectNameEXT == nullptr || name == nullptr) {
        return;
    }
    VkDebugUtilsObjectNameInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = type;
    info.objectHandle = handle;
    info.pObjectName = name;
    vkSetDebugUtilsObjectNameEXT(device, &info);
}

void BeginDebugLabel(VkCommandBuffer cmd, const char* name, float r, float g, float b) {
    if (!g_DebugUtilsEnabled || vkCmdBeginDebugUtilsLabelEXT == nullptr) {
        return;
    }
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name;
    label.color[0] = r;
    label.color[1] = g;
    label.color[2] = b;
    label.color[3] = 1.0f;
    vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
}

void EndDebugLabel(VkCommandBuffer cmd) {
    if (!g_DebugUtilsEnabled || vkCmdEndDebugUtilsLabelEXT == nullptr) {
        return;
    }
    vkCmdEndDebugUtilsLabelEXT(cmd);
}

void InsertDebugLabel(VkCommandBuffer cmd, const char* name) {
    if (!g_DebugUtilsEnabled || vkCmdInsertDebugUtilsLabelEXT == nullptr) {
        return;
    }
    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = name;
    vkCmdInsertDebugUtilsLabelEXT(cmd, &label);
}

} // namespace Hockey
