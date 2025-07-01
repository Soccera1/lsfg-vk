#include "vulkan/funcs.hpp"
#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "log.hpp"

using namespace Vulkan;

namespace {
    PFN_vkCreateInstance vkCreateInstance_ptr{};
    PFN_vkDestroyInstance vkDestroyInstance_ptr{};

    PFN_vkCreateDevice vkCreateDevice_ptr{};
    PFN_vkDestroyDevice vkDestroyDevice_ptr{};
}

void Funcs::initialize() {
    if (vkCreateInstance_ptr || vkDestroyInstance_ptr) {
        Log::warn("lsfg-vk(funcs): Global Vulkan functions already initialized, did you call it twice?");
        return;
    }

    auto* handle = Loader::DL::odlopen("libvulkan.so.1", 0x2);
    vkCreateInstance_ptr = reinterpret_cast<PFN_vkCreateInstance>(
        Loader::DL::odlsym(handle, "vkCreateInstance"));
    vkDestroyInstance_ptr = reinterpret_cast<PFN_vkDestroyInstance>(
        Loader::DL::odlsym(handle, "vkDestroyInstance"));
    if (!vkCreateInstance_ptr || !vkDestroyInstance_ptr) {
        Log::error("lsfg-vk(funcs): Failed to initialize Vulkan functions, missing symbols");
        exit(EXIT_FAILURE);
    }

    Log::debug("lsfg-vk(funcs): Initialized global Vulkan functions with original pointers");
}

void Funcs::initializeInstance(VkInstance instance) {
    if (vkCreateDevice_ptr || vkDestroyDevice_ptr) {
        Log::warn("lsfg-vk(funcs): Instance Vulkan functions already initialized, did you call it twice?");
        return;
    }

    vkCreateDevice_ptr = reinterpret_cast<PFN_vkCreateDevice>(
        Loader::VK::ovkGetInstanceProcAddr(instance, "vkCreateDevice"));
    vkDestroyDevice_ptr = reinterpret_cast<PFN_vkDestroyDevice>(
        Loader::VK::ovkGetInstanceProcAddr(instance, "vkDestroyDevice"));
    if (!vkCreateDevice_ptr || !vkDestroyDevice_ptr) {
        Log::error("lsfg-vk(funcs): Failed to initialize Vulkan instance functions, missing symbols");
        exit(EXIT_FAILURE);
    }

    Log::debug("lsfg-vk(funcs): Initialized instance Vulkan functions with original pointers");
}

void Funcs::initializeDevice(VkDevice device) {
    Log::debug("lsfg-vk(funcs): Initialized device Vulkan functions with original pointers");
}

// original function calls

VkResult Funcs::ovkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {
    return vkCreateInstance_ptr(pCreateInfo, pAllocator, pInstance);
}

void Funcs::ovkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator) {
    vkDestroyInstance_ptr(instance, pAllocator);
}

VkResult Funcs::ovkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice) {
    return vkCreateDevice_ptr(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

void Funcs::ovkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator) {
    vkDestroyDevice_ptr(device, pAllocator);
}
