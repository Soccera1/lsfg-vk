#ifndef FUNCS_HPP
#define FUNCS_HPP

#include <vulkan/vulkan_core.h>

namespace Vulkan::Funcs {

    ///
    /// Initialize the global Vulkan function pointers.
    ///
    void initialize();

    ///
    /// Initialize the instance Vulkan function pointers.
    ///
    /// @param instance The Vulkan instance to initialize.
    ///
    void initializeInstance(VkInstance instance);

    ///
    /// Initialize the device Vulkan function pointers.
    ///
    /// @param device The Vulkan device to initialize.
    ///
    void initializeDevice(VkDevice device);

    /// Call to the original vkCreateInstance function.
    VkResult ovkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance
    );
    /// Call to the original vkDestroyInstance function.
    void ovkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator
    );

    /// Call to the original vkCreateDevice function.
    VkResult ovkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice
    );
    /// Call to the original vkDestroyDevice function.
    void ovkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator
    );
}

#endif // FUNCS_HPP
