#include "hooks.hpp"
#include "context.hpp"
#include "layer.hpp"
#include "utils/log.hpp"
#include "utils/utils.hpp"

#include <lsfg.hpp>

#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

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

        Log::info("lsfg-vk: Created Vulkan instance");
        VkInstanceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        return Layer::ovkCreateInstance(&createInfo, pAllocator, pInstance);
    }

    void myvkDestroyInstance(
            VkInstance instance,
            const VkAllocationCallbacks* pAllocator) {
        Log::info("lsfg-vk: Destroyed Vulkan instance");
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
        return Layer::ovkCreateDevice(physicalDevice, &createInfo, pAllocator, pDevice);
    }

    VkResult myvkCreateDevicePost(
            VkPhysicalDevice physicalDevice,
            VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        // store device info
        try {
            const char* frameGen = std::getenv("LSFG_MULTIPLIER");
            if (!frameGen) frameGen = "2";
            devices.emplace(*pDevice, DeviceInfo {
                .device = *pDevice,
                .physicalDevice = physicalDevice,
                .queue = Utils::findQueue(*pDevice, physicalDevice, pCreateInfo,
                    VK_QUEUE_GRAPHICS_BIT),
                .frameGen = std::max<size_t>(1, std::stoul(frameGen) - 1)
            });
        } catch (const std::exception& e) {
            Log::error("Failed to create device info: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        Log::info("lsfg-vk: Created Vulkan device");
        return VK_SUCCESS;
    }

    void myvkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
        devices.erase(device); // erase device info

        Log::info("lsfg-vk: Destroyed Vulkan device");
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
        auto& deviceInfo = devices.at(device);

        // update swapchain create info
        VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
        createInfo.minImageCount += 1 + deviceInfo.frameGen; // 1 deferred + N framegen, FIXME: check hardware max
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // allow copy from/to images
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // force vsync
        auto res = Layer::ovkCreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
        if (res != VK_SUCCESS) {
            Log::error("Failed to create swapchain: {:x}", static_cast<uint32_t>(res));
            return res;
        }

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

            // create swapchain context
            swapchains.emplace(*pSwapchain, LsContext(
                deviceInfo, *pSwapchain, pCreateInfo->imageExtent,
                swapchainImages
            ));

            swapchainToDeviceTable.emplace(*pSwapchain, device);
        } catch (const LSFG::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while creating swapchain: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error();
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating swapchain: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        Log::info("lsfg-vk: Created swapchain with {} images", pCreateInfo->minImageCount);
        return res;
    }

    VkResult myvkQueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo) {
        auto& deviceInfo = devices.at(swapchainToDeviceTable.at(*pPresentInfo->pSwapchains));
        auto& swapchain = swapchains.at(*pPresentInfo->pSwapchains);

        try {
            std::vector<VkSemaphore> waitSemaphores(pPresentInfo->waitSemaphoreCount);
            std::copy_n(pPresentInfo->pWaitSemaphores, waitSemaphores.size(), waitSemaphores.data());

            // present the next frame
            return swapchain.present(deviceInfo, pPresentInfo->pNext,
                queue, waitSemaphores, *pPresentInfo->pImageIndices);
        } catch (const LSFG::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while presenting: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error();
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating presenting: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    void myvkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator) {
        swapchains.erase(swapchain); // erase swapchain context
        swapchainToDeviceTable.erase(swapchain);

        Log::info("lsfg-vk: Destroyed swapchain");
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
