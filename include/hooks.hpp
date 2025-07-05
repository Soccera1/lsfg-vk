#ifndef HOOKS_HPP
#define HOOKS_HPP

#include <vulkan/vulkan_core.h>

#include <unordered_map>
#include <utility>
#include <string>

namespace Hooks {

    /// Vulkan device information structure.
    struct DeviceInfo {
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        std::pair<uint32_t, VkQueue> queue; // graphics family
        uint64_t frameGen; // amount of frames to generate
    };

    /// Map of hooked Vulkan functions.
    extern std::unordered_map<std::string, PFN_vkVoidFunction> hooks;

}

#endif // HOOKS_HPP
