#include "hooks.hpp"
#include "context.hpp"
#include "layer.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

#include <lsfg.hpp>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

using namespace Hooks;

namespace {

    // instance hooks

    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        // add extensions
        auto extensions = Utils::addExtensions(pCreateInfo->ppEnabledExtensionNames,
            pCreateInfo->enabledExtensionCount, {
                "VK_KHR_get_physical_device_properties2",
                "VK_KHR_external_memory_capabilities",
                "VK_KHR_external_semaphore_capabilities"
            });
        VkInstanceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        auto res = Layer::ovkCreateInstance(&createInfo, pAllocator, pInstance);
        if (res != VK_SUCCESS) {
            Log::error("hooks", "Failed to create Vulkan instance: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        Log::info("hooks", "Instance created successfully: {:x}",
            reinterpret_cast<uintptr_t>(*pInstance));
        return res;
    }

    void myvkDestroyInstance(
            VkInstance instance,
            const VkAllocationCallbacks* pAllocator) {
        Log::info("hooks", "Instance destroyed successfully: {:x}",
            reinterpret_cast<uintptr_t>(instance));
        Layer::ovkDestroyInstance(instance, pAllocator);
    }

    // device hooks

    std::unordered_map<VkDevice, DeviceInfo> devices;

    VkResult myvkCreateDevicePre(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        // add extensions
        auto extensions = Utils::addExtensions(pCreateInfo->ppEnabledExtensionNames,
            pCreateInfo->enabledExtensionCount, {
                "VK_KHR_external_memory",
                "VK_KHR_external_memory_fd",
                "VK_KHR_external_semaphore",
                "VK_KHR_external_semaphore_fd"
            });

        VkDeviceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        auto res = Layer::ovkCreateDevice(physicalDevice, &createInfo, pAllocator, pDevice);
        if (res != VK_SUCCESS) {
            Log::error("hooks", "Failed to create Vulkan device: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        Log::info("hooks", "Device created successfully: {:x}",
            reinterpret_cast<uintptr_t>(*pDevice));
        return res;
    }

    VkResult myvkCreateDevicePost(
            VkPhysicalDevice physicalDevice,
            VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks*,
            VkDevice* pDevice) {
        // store device info
        Log::debug("hooks", "Creating device info for device: {:x}",
            reinterpret_cast<uintptr_t>(*pDevice));
        try {
            const char* frameGenEnv = std::getenv("LSFG_MULTIPLIER");
            const uint64_t frameGen = static_cast<uint64_t>(
                std::max<int64_t>(1, std::stol(frameGenEnv ? frameGenEnv : "2") - 1));
            Log::debug("hooks", "Using {}x frame generation",
                frameGen + 1);

            auto queue = Utils::findQueue(*pDevice, physicalDevice, pCreateInfo,
                VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
            Log::debug("hooks", "Found queue at index {}: {:x}",
                queue.first, reinterpret_cast<uintptr_t>(queue.second));

            devices.emplace(*pDevice, DeviceInfo {
                .device = *pDevice,
                .physicalDevice = physicalDevice,
                .queue = queue,
                .frameGen = frameGen,
            });
        } catch (const std::exception& e) {
            Log::error("hooks", "Failed to create device info: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        Log::info("hooks", "Device info created successfully for: {:x}",
            reinterpret_cast<uintptr_t>(*pDevice));
        return VK_SUCCESS;
    }

    void myvkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
        devices.erase(device); // erase device info

        Log::info("hooks", "Device & Device info destroyed successfully: {:x}",
            reinterpret_cast<uintptr_t>(device));
        Layer::ovkDestroyDevice(device, pAllocator);
    }

    // swapchain hooks

    std::unordered_map<VkSwapchainKHR, LsContext> swapchains;
    std::unordered_map<VkSwapchainKHR, VkDevice> swapchainToDeviceTable;

    VkResult myvkCreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain) {
        auto it = devices.find(device);
        if (it == devices.end()) {
            Log::warn("hooks", "Created swapchain without device info present");
            return Layer::ovkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
        }
        auto& deviceInfo = it->second;

        // update swapchain create info
        VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
        const uint32_t maxImageCount = Utils::getMaxImageCount(deviceInfo.physicalDevice, pCreateInfo->surface);
        const uint32_t imageCount = createInfo.minImageCount + 1 + static_cast<uint32_t>(deviceInfo.frameGen);
        Log::debug("hooks", "Creating swapchain with max image count: {}/{}", imageCount, maxImageCount);
        if (imageCount > maxImageCount) {
            Log::warn("hooks", "LSFG_MULTIPLIER is set very high. This might lead to performance degradation");
            createInfo.minImageCount = maxImageCount; // limit to max possible
        } else {
            createInfo.minImageCount = imageCount; // set to frameGen + 1
        }
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // allow copy from/to images
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // force vsync
        auto res = Layer::ovkCreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
        if (res != VK_SUCCESS) {
            Log::error("hooks", "Failed to create swapchain: {}", static_cast<uint32_t>(res));
            return res;
        }
        Log::info("hooks", "Swapchain created successfully: {:x}",
            reinterpret_cast<uintptr_t>(*pSwapchain));

        // retire previous swapchain if it exists
        if (pCreateInfo->oldSwapchain) {
            Log::debug("hooks", "Retiring previous swapchain context: {:x}",
                reinterpret_cast<uintptr_t>(pCreateInfo->oldSwapchain));
            swapchains.erase(pCreateInfo->oldSwapchain);
            swapchainToDeviceTable.erase(pCreateInfo->oldSwapchain);
            Log::info("hooks", "Previous swapchain context retired successfully: {:x}",
                reinterpret_cast<uintptr_t>(pCreateInfo->oldSwapchain));
        }

        // create swapchain context
        Log::debug("hooks", "Creating swapchain context for device: {:x}",
            reinterpret_cast<uintptr_t>(device));
        try {
            // get swapchain images
            uint32_t imageCount{};
            res = Layer::ovkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr);
            if (res != VK_SUCCESS || imageCount == 0)
                throw LSFG::vulkan_error(res, "Failed to get swapchain images count");

            std::vector<VkImage> swapchainImages(imageCount);
            res = Layer::ovkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, swapchainImages.data());
            if (res != VK_SUCCESS)
                throw LSFG::vulkan_error(res, "Failed to get swapchain images");
            Log::debug("hooks", "Swapchain has {} images", swapchainImages.size());

            // create swapchain context
            swapchains.emplace(*pSwapchain, LsContext(
                deviceInfo, *pSwapchain, pCreateInfo->imageExtent,
                swapchainImages
            ));
            swapchainToDeviceTable.emplace(*pSwapchain, device);
        } catch (const LSFG::vulkan_error& e) {
            Log::error("hooks", "Encountered Vulkan error {:x} while creating swapchain context: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error();
        } catch (const std::exception& e) {
            Log::error("hooks", "Encountered error while creating swapchain context: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        Log::info("hooks", "Swapchain context created successfully for: {:x}",
            reinterpret_cast<uintptr_t>(*pSwapchain));
        return res;
    }

    VkResult myvkQueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo) {
        auto it = swapchainToDeviceTable.find(*pPresentInfo->pSwapchains);
        if (it == swapchainToDeviceTable.end()) {
            Log::warn("hooks2", "Swapchain {:x} not found in swapchainToDeviceTable",
                reinterpret_cast<uintptr_t>(*pPresentInfo->pSwapchains));
            return Layer::ovkQueuePresentKHR(queue, pPresentInfo);
        }
        auto it2 = devices.find(it->second);
        if (it2 == devices.end()) {
            Log::warn("hooks2", "Device {:x} not found in devices",
                reinterpret_cast<uintptr_t>(it->second));
            return Layer::ovkQueuePresentKHR(queue, pPresentInfo);
        }
        auto it3 = swapchains.find(*pPresentInfo->pSwapchains);
        if (it3 == swapchains.end()) {
            Log::warn("hooks2", "Swapchain {:x} not found in swapchains",
                reinterpret_cast<uintptr_t>(*pPresentInfo->pSwapchains));
            return Layer::ovkQueuePresentKHR(queue, pPresentInfo);
        }

        auto& deviceInfo = it2->second;
        auto& swapchain = it3->second;

        // patch vsync NOLINTBEGIN
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        const VkSwapchainPresentModeInfoEXT* presentModeInfo =
            reinterpret_cast<const VkSwapchainPresentModeInfoEXT*>(pPresentInfo->pNext);
        while (presentModeInfo) {
            if (presentModeInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT) {
                for (size_t i = 0; i < presentModeInfo->swapchainCount; i++)
                    const_cast<VkPresentModeKHR*>(presentModeInfo->pPresentModes)[i] =
                        VK_PRESENT_MODE_FIFO_KHR;
            }
            presentModeInfo =
                reinterpret_cast<const VkSwapchainPresentModeInfoEXT*>(presentModeInfo->pNext);
        } // NOLINTEND
#pragma clang diagnostic pop

        // present the next frame
        Log::debug("hooks2", "Presenting swapchain: {:x} on queue: {:x}",
            reinterpret_cast<uintptr_t>(*pPresentInfo->pSwapchains),
            reinterpret_cast<uintptr_t>(queue));
        VkResult res{};
        try {
            std::vector<VkSemaphore> semaphores(pPresentInfo->waitSemaphoreCount);
            std::copy_n(pPresentInfo->pWaitSemaphores, semaphores.size(), semaphores.data());
            Log::debug("hooks2", "Waiting on {} semaphores", semaphores.size());

            // present the next frame
            res = swapchain.present(deviceInfo, pPresentInfo->pNext,
                queue, semaphores, *pPresentInfo->pImageIndices);
        } catch (const LSFG::vulkan_error& e) {
            Log::error("hooks2", "Encountered Vulkan error {:x} while presenting: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error();
        } catch (const std::exception& e) {
            Log::error("hooks2", "Encountered error while creating presenting: {}",
                e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // non VK_SUCCESS or VK_SUBOPTIMAL_KHR doesn't reach here
        Log::debug("hooks2", "Presented swapchain {:x} on queue {:x} successfully",
            reinterpret_cast<uintptr_t>(*pPresentInfo->pSwapchains),
            reinterpret_cast<uintptr_t>(queue));
        return res;
    }

    void myvkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator) {
        swapchains.erase(swapchain); // erase swapchain context
        swapchainToDeviceTable.erase(swapchain);

        Log::info("hooks", "Swapchain & Swapchain context destroyed successfully: {:x}",
            reinterpret_cast<uintptr_t>(swapchain));
        Layer::ovkDestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

std::unordered_map<std::string, PFN_vkVoidFunction> Hooks::hooks = {
    // instance hooks
    {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateInstance)},
    {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(myvkDestroyInstance)},

    // device hooks
    {"vkCreateDevicePre", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateDevicePre)},
    {"vkCreateDevicePost", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateDevicePost)},
    {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(myvkDestroyDevice)},

    // swapchain hooks
    {"vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(myvkCreateSwapchainKHR)},
    {"vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(myvkQueuePresentKHR)},
    {"vkDestroySwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(myvkDestroySwapchainKHR)}
};
