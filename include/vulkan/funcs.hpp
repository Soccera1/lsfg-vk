#ifndef FUNCS_HPP
#define FUNCS_HPP

#include <vulkan/vulkan_core.h>

namespace Vulkan::Funcs {

    ///
    /// Initialize the global Vulkan function pointers.
    ///
    void initialize();

    /// Call to the original vkCreateInstance function.
    VkResult vkCreateInstanceOriginal(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance
    );
    /// Call to the original vkDestroyInstance function.
    void vkDestroyInstanceOriginal(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator
    );
}

#endif // FUNCS_HPP
