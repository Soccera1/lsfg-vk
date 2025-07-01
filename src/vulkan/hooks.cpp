#include "vulkan/hooks.hpp"
#include "vulkan/funcs.hpp"
#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "application.hpp"
#include "log.hpp"

#include <lsfg.hpp>

#include <optional>

using namespace Vulkan;

namespace {
    bool initialized{false};
    std::optional<Application> application;

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

        // create the main application
        if (application.has_value()) {
            Log::error("Application already initialized, are you trying to create a second device?");
            exit(EXIT_FAILURE);
        }

        try {
            application.emplace(*pDevice, physicalDevice);
            Log::info("lsfg-vk(hooks): Application created successfully");
        } catch (const LSFG::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while creating application: {}",
                static_cast<uint32_t>(e.error()), e.what());
            exit(EXIT_FAILURE);
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating application: {}", e.what());
            exit(EXIT_FAILURE);
        }

        return res;
    }

    void myvkDestroyDevice(
            VkDevice device,
            const VkAllocationCallbacks* pAllocator) {
        // destroy the main application
        if (application.has_value()) {
            application.reset();
            Log::info("lsfg-vk(hooks): Application destroyed successfully");
        } else {
            Log::warn("lsfg-vk(hooks): No application to destroy, continuing");
        }

        Funcs::ovkDestroyDevice(device, pAllocator);
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
    Loader::VK::registerSymbol("vkDestroyDevice",
        reinterpret_cast<void*>(myvkDestroyDevice));

    // register hooks to dynamic loader under libvulkan.so.1
    Loader::DL::File vk1("libvulkan.so.1");
    vk1.defineSymbol("vkCreateInstance",
        reinterpret_cast<void*>(myvkCreateInstance));
    vk1.defineSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    vk1.defineSymbol("vkDestroyDevice",
        reinterpret_cast<void*>(myvkDestroyDevice));
    Loader::DL::registerFile(vk1);

    // register hooks to dynamic loader under libvulkan.so
    Loader::DL::File vk2("libvulkan.so");
    vk2.defineSymbol("vkCreateInstance",
        reinterpret_cast<void*>(myvkCreateInstance));
    vk2.defineSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    vk2.defineSymbol("vkDestroyDevice",
        reinterpret_cast<void*>(myvkDestroyDevice));
    Loader::DL::registerFile(vk2);
}
