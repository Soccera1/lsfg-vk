#include "hooks.hpp"
#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "application.hpp"
#include "log.hpp"

#include <lsfg.hpp>

#include <optional>

using namespace Hooks;

namespace {
    bool initialized{false};
    std::optional<Application> application;

    VkResult myvkCreateDevice(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        auto res = vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

        // extract graphics and present queues
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(pCreateInfo->queueCreateInfoCount);
        std::copy_n(pCreateInfo->pQueueCreateInfos, queueCreateInfos.size(), queueCreateInfos.data());

        uint32_t familyCount{};
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

        std::optional<uint32_t> graphicsFamilyIdx;
        std::optional<uint32_t> presentFamilyIdx;
        for (uint32_t i = 0; i < families.size(); ++i) {
            auto it = std::ranges::find_if(queueCreateInfos,
                [i](const VkDeviceQueueCreateInfo& info) {
                    return info.queueFamilyIndex == i;
                }) ;
            if (it == queueCreateInfos.end())
                continue; // skip if this family is not used by the device
            if (families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
                graphicsFamilyIdx.emplace(i);
            if (families.at(i).queueFlags & VK_QUEUE_COMPUTE_BIT)
                presentFamilyIdx.emplace(i);
        }
        if (!graphicsFamilyIdx.has_value() || !presentFamilyIdx.has_value()) {
            Log::error("No suitable queue family found for graphics or present");
            exit(EXIT_FAILURE);
        }

        VkQueue graphicsQueue{};
        vkGetDeviceQueue(*pDevice, *graphicsFamilyIdx, 0, &graphicsQueue);
        VkQueue presentQueue{};
        vkGetDeviceQueue(*pDevice, *presentFamilyIdx, 0, &presentQueue);

        // create the main application
        if (application.has_value()) {
            Log::error("Application already initialized, are you trying to create a second device?");
            exit(EXIT_FAILURE);
        }

        try {
            application.emplace(*pDevice, physicalDevice, graphicsQueue, presentQueue);
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

    VkResult myvkCreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain) {
        auto res = vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

        // add the swapchain to the application
        if (!application.has_value()) {
            Log::error("Application not initialized, cannot create swapchain");
            exit(EXIT_FAILURE);
        }

        try {
            if (pCreateInfo->oldSwapchain) {
                if (!application->removeSwapchain(pCreateInfo->oldSwapchain))
                    throw std::runtime_error("Failed to remove old swapchain");
                Log::info("lsfg-vk(hooks): Swapchain retired successfully");
            }

            uint32_t imageCount{};
            auto res = vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr);
            if (res != VK_SUCCESS || imageCount == 0)
                throw LSFG::vulkan_error(res, "Failed to get swapchain images count");

            std::vector<VkImage> swapchainImages(imageCount);
            res = vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, swapchainImages.data());
            if (res != VK_SUCCESS)
                throw LSFG::vulkan_error(res, "Failed to get swapchain images");

            application->addSwapchain(*pSwapchain,
                pCreateInfo->imageFormat, pCreateInfo->imageExtent, swapchainImages);
            Log::info("lsfg-vk(hooks): Swapchain created successfully with {} images",
                swapchainImages.size());
        } catch (const LSFG::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while creating swapchain: {}",
                static_cast<uint32_t>(e.error()), e.what());
            exit(EXIT_FAILURE);
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating swapchain: {}", e.what());
            exit(EXIT_FAILURE);
        }

        return res;
    }

    VkResult myvkQueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo) {
        if (!application.has_value()) {
            Log::error("Application not initialized, cannot present frame");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // present the next frame
        try {
            std::vector<VkSemaphore> waitSemaphores(pPresentInfo->waitSemaphoreCount);
            std::copy_n(pPresentInfo->pWaitSemaphores, waitSemaphores.size(), waitSemaphores.data());

            application->presentSwapchain(*pPresentInfo->pSwapchains,
                queue, waitSemaphores, *pPresentInfo->pImageIndices);

            Log::info("lsfg-vk(hooks): Frame presented successfully");
        } catch (const LSFG::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while presenting: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error(); // do not exit
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating presenting: {}", e.what());
            exit(EXIT_FAILURE);
        }

        return VK_SUCCESS;
    }


    void myvkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator) {
        if (!application.has_value()) {
            Log::error("Application not initialized, cannot destroy swapchain");
            exit(EXIT_FAILURE);
        }

        // remove the swapchain from the application
        try {
            if (application->removeSwapchain(swapchain))
                Log::info("lsfg-vk(hooks): Swapchain retired successfully");
        } catch (const std::exception& e) {
            Log::error("Encountered error while removing swapchain: {}", e.what());
            exit(EXIT_FAILURE);
        }

        vkDestroySwapchainKHR(device, swapchain, pAllocator);
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

        vkDestroyDevice(device, pAllocator);
    }

}

void Hooks::initialize() {
    if (initialized) {
        Log::warn("Vulkan hooks already initialized, did you call it twice?");
        return;
    }

    // register hooks to vulkan loader
    Loader::VK::registerSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    Loader::VK::registerSymbol("vkDestroyDevice",
        reinterpret_cast<void*>(myvkDestroyDevice));
    Loader::VK::registerSymbol("vkCreateSwapchainKHR",
        reinterpret_cast<void*>(myvkCreateSwapchainKHR));
    Loader::VK::registerSymbol("vkDestroySwapchainKHR",
        reinterpret_cast<void*>(myvkDestroySwapchainKHR));
    Loader::VK::registerSymbol("vkQueuePresentKHR",
        reinterpret_cast<void*>(myvkQueuePresentKHR));

    // register hooks to dynamic loader under libvulkan.so.1
    Loader::DL::File vk1("libvulkan.so.1");
    vk1.defineSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    vk1.defineSymbol("vkDestroyDevice",
        reinterpret_cast<void*>(myvkDestroyDevice));
    vk1.defineSymbol("vkCreateSwapchainKHR",
        reinterpret_cast<void*>(myvkCreateSwapchainKHR));
    vk1.defineSymbol("vkDestroySwapchainKHR",
        reinterpret_cast<void*>(myvkDestroySwapchainKHR));
    vk1.defineSymbol("vkQueuePresentKHR",
        reinterpret_cast<void*>(myvkQueuePresentKHR));
    Loader::DL::registerFile(vk1);

    // register hooks to dynamic loader under libvulkan.so
    Loader::DL::File vk2("libvulkan.so");
    vk2.defineSymbol("vkCreateDevice",
        reinterpret_cast<void*>(myvkCreateDevice));
    vk2.defineSymbol("vkDestroyDevice",
        reinterpret_cast<void*>(myvkDestroyDevice));
    vk2.defineSymbol("vkCreateSwapchainKHR",
        reinterpret_cast<void*>(myvkCreateSwapchainKHR));
    vk2.defineSymbol("vkDestroySwapchainKHR",
        reinterpret_cast<void*>(myvkDestroySwapchainKHR));
    vk2.defineSymbol("vkQueuePresentKHR",
        reinterpret_cast<void*>(myvkQueuePresentKHR));
    Loader::DL::registerFile(vk2);
}
