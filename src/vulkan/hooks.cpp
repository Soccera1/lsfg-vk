#include "vulkan/hooks.hpp"
#include "vulkan/funcs.hpp"
#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "log.hpp"

#include <vulkan/vulkan_core.h>

using namespace Vulkan;

namespace {
    bool initialized{false};

    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        auto res = Funcs::ovkCreateInstance(pCreateInfo, pAllocator, pInstance);

        Funcs::initializeInstance(*pInstance);

        return res;
    }

    VkResult myvkCreateDevice(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        auto res = Funcs::ovkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

        Funcs::initializeDevice(*pDevice);

        return res;
    }

}

void Hooks::initialize() {
    if (initialized) {
        Log::warn("Vulkan hooks already initialized, did you call it twice?");
        return;
    }

    // register hooks to vulkan loader
    Loader::VK::registerSymbol("vkCreateInstance",
        reinterpret_cast<void*>(myvkCreateInstance));
    Loader::VK::registerSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));

    // register hooks to dynamic loader under libvulkan.so.1
    Loader::DL::File vk1("libvulkan.so.1");
    vk1.defineSymbol("vkCreateInstance",
        reinterpret_cast<void*>(myvkCreateInstance));
    vk1.defineSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    Loader::DL::registerFile(vk1);

    // register hooks to dynamic loader under libvulkan.so
    Loader::DL::File vk2("libvulkan.so");
    vk2.defineSymbol("vkCreateInstance",
        reinterpret_cast<void*>(myvkCreateInstance));
    vk2.defineSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    Loader::DL::registerFile(vk2);
}
