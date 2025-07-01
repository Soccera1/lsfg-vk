#include "vulkan/funcs.hpp"
#include "loader/dl.hpp"
#include "log.hpp"

using namespace Vulkan;

namespace {
    PFN_vkCreateInstance vkCreateInstance_ptr{};
    PFN_vkDestroyInstance vkDestroyInstance_ptr{};
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

// original function calls

VkResult Funcs::vkCreateInstanceOriginal(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {
    return vkCreateInstance_ptr(pCreateInfo, pAllocator, pInstance);
}

void Funcs::vkDestroyInstanceOriginal(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator) {
    vkDestroyInstance_ptr(instance, pAllocator);
}
