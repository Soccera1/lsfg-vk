#include "layer.hpp"
#include "hooks.hpp"
#include "utils/log.hpp"

#include <lsfg.hpp>
#include <vulkan/vk_layer.h>

#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace {
    PFN_vkCreateInstance  next_vkCreateInstance{};
    PFN_vkDestroyInstance next_vkDestroyInstance{};

    PFN_vkCreateDevice  next_vkCreateDevice{};
    PFN_vkDestroyDevice next_vkDestroyDevice{};

    PFN_vkGetInstanceProcAddr next_vkGetInstanceProcAddr{};
    PFN_vkGetDeviceProcAddr   next_vkGetDeviceProcAddr{};

    PFN_vkCreateSwapchainKHR  next_vkCreateSwapchainKHR{};
    PFN_vkQueuePresentKHR     next_vkQueuePresentKHR{};
    PFN_vkDestroySwapchainKHR next_vkDestroySwapchainKHR{};
    PFN_vkGetSwapchainImagesKHR  next_vkGetSwapchainImagesKHR{};
    PFN_vkAllocateCommandBuffers next_vkAllocateCommandBuffers{};
    PFN_vkFreeCommandBuffers     next_vkFreeCommandBuffers{};
    PFN_vkBeginCommandBuffer next_vkBeginCommandBuffer{};
    PFN_vkEndCommandBuffer   next_vkEndCommandBuffer{};
    PFN_vkCreateCommandPool  next_vkCreateCommandPool{};
    PFN_vkDestroyCommandPool next_vkDestroyCommandPool{};
    PFN_vkCreateImage  next_vkCreateImage{};
    PFN_vkDestroyImage next_vkDestroyImage{};
    PFN_vkGetImageMemoryRequirements next_vkGetImageMemoryRequirements{};
    PFN_vkBindImageMemory next_vkBindImageMemory{};
    PFN_vkAllocateMemory  next_vkAllocateMemory{};
    PFN_vkFreeMemory next_vkFreeMemory{};
    PFN_vkCreateSemaphore  next_vkCreateSemaphore{};
    PFN_vkDestroySemaphore next_vkDestroySemaphore{};
    PFN_vkGetMemoryFdKHR next_vkGetMemoryFdKHR{};
    PFN_vkGetSemaphoreFdKHR next_vkGetSemaphoreFdKHR{};
    PFN_vkGetPhysicalDeviceQueueFamilyProperties next_vkGetPhysicalDeviceQueueFamilyProperties{};
    PFN_vkGetPhysicalDeviceMemoryProperties next_vkGetPhysicalDeviceMemoryProperties{};
    PFN_vkGetDeviceQueue next_vkGetDeviceQueue{};
    PFN_vkQueueSubmit next_vkQueueSubmit{};
    PFN_vkCmdPipelineBarrier next_vkCmdPipelineBarrier{};
    PFN_vkCmdPipelineBarrier2 next_vkCmdPipelineBarrier2{};
    PFN_vkCmdCopyImage next_vkCmdCopyImage{};
    PFN_vkAcquireNextImageKHR next_vkAcquireNextImageKHR{};
}

namespace {
    VkResult layer_vkCreateInstance( // NOLINTBEGIN
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        Log::debug("lsfg-vk(layer): Initializing lsfg-vk instance layer");

        // find layer creation info
        auto* layerDesc = const_cast<VkLayerInstanceCreateInfo*>(
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext));
        while (layerDesc && (layerDesc->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
                || layerDesc->function != VK_LAYER_LINK_INFO)) {
            layerDesc = const_cast<VkLayerInstanceCreateInfo*>(
                reinterpret_cast<const VkLayerInstanceCreateInfo*>(layerDesc->pNext));
        }
        if (!layerDesc) {
            Log::error("lsfg-vk(layer): No layer creation info found in pNext chain");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // advance link info (i don't really know what this does)
        next_vkGetInstanceProcAddr = layerDesc->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        layerDesc->u.pLayerInfo = layerDesc->u.pLayerInfo->pNext;

        // create instance
        next_vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
            next_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
        if (!next_vkCreateInstance) {
            Log::error("lsfg-vk(layer): Failed to get vkCreateInstance function pointer");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto* layer_vkCreateInstance2 = reinterpret_cast<PFN_vkCreateInstance>(
            Hooks::hooks["vkCreateInstance"]);
        auto res = layer_vkCreateInstance2(pCreateInfo, pAllocator, pInstance);
        if (res != VK_SUCCESS) {
            Log::error("lsfg-vk(layer): Failed to create Vulkan instance: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        // get relevant function pointers from the next layer
        next_vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
            next_vkGetInstanceProcAddr(*pInstance, "vkDestroyInstance"));
        next_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            next_vkGetInstanceProcAddr(*pInstance, "vkGetInstanceProcAddr"));
        next_vkGetPhysicalDeviceQueueFamilyProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
                next_vkGetInstanceProcAddr(*pInstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
        next_vkGetPhysicalDeviceMemoryProperties =
            reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
                next_vkGetInstanceProcAddr(*pInstance, "vkGetPhysicalDeviceMemoryProperties"));
        if (!next_vkDestroyInstance || !next_vkGetInstanceProcAddr ||
            !next_vkGetPhysicalDeviceQueueFamilyProperties ||
            !next_vkGetPhysicalDeviceMemoryProperties) {
            Log::error("lsfg-vk(layer): Failed to get instance function pointers");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        Log::debug("lsfg-vk(layer): Successfully initialized lsfg-vk instance layer");
        return res;
    } // NOLINTEND

    VkResult layer_vkCreateDevice( // NOLINTBEGIN
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        Log::debug("lsfg-vk(layer): Initializing lsfg-vk device layer");

        // find layer creation info
        auto* layerDesc = const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
        while (layerDesc && (layerDesc->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                || layerDesc->function != VK_LAYER_LINK_INFO)) {
            layerDesc = const_cast<VkLayerDeviceCreateInfo*>(
                reinterpret_cast<const VkLayerDeviceCreateInfo*>(layerDesc->pNext));
        }
        if (!layerDesc) {
            Log::error("lsfg-vk(layer): No layer creation info found in pNext chain");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // advance link info (i don't really know what this does)
        next_vkGetDeviceProcAddr = layerDesc->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        layerDesc->u.pLayerInfo = layerDesc->u.pLayerInfo->pNext;

        // create device
        next_vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
            next_vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateDevice"));
        if (!next_vkCreateDevice) {
            Log::error("lsfg-vk(layer): Failed to get vkCreateDevice function pointer");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        auto* layer_vkCreateDevice2 = reinterpret_cast<PFN_vkCreateDevice>(
            Hooks::hooks["vkCreateDevicePre"]);
        auto res = layer_vkCreateDevice2(physicalDevice, pCreateInfo, pAllocator, pDevice);
        if (res != VK_SUCCESS) {
            Log::error("lsfg-vk(layer): Failed to create Vulkan device: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        // get relevant function pointers from the next layer
        next_vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(
            next_vkGetDeviceProcAddr(*pDevice, "vkDestroyDevice"));
        next_vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCreateSwapchainKHR"));
        next_vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkQueuePresentKHR"));
        next_vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkDestroySwapchainKHR"));
        next_vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkGetSwapchainImagesKHR"));
        next_vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(
            next_vkGetDeviceProcAddr(*pDevice, "vkAllocateCommandBuffers"));
        next_vkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(
            next_vkGetDeviceProcAddr(*pDevice, "vkFreeCommandBuffers"));
        next_vkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(
            next_vkGetDeviceProcAddr(*pDevice, "vkBeginCommandBuffer"));
        next_vkEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(
            next_vkGetDeviceProcAddr(*pDevice, "vkEndCommandBuffer"));
        next_vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCreateCommandPool"));
        next_vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(
            next_vkGetDeviceProcAddr(*pDevice, "vkDestroyCommandPool"));
        next_vkCreateImage = reinterpret_cast<PFN_vkCreateImage>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCreateImage"));
        next_vkDestroyImage = reinterpret_cast<PFN_vkDestroyImage>(
            next_vkGetDeviceProcAddr(*pDevice, "vkDestroyImage"));
        next_vkGetImageMemoryRequirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(
            next_vkGetDeviceProcAddr(*pDevice, "vkGetImageMemoryRequirements"));
        next_vkBindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(
            next_vkGetDeviceProcAddr(*pDevice, "vkBindImageMemory"));
        next_vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkGetMemoryFdKHR"));
        next_vkAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(
            next_vkGetDeviceProcAddr(*pDevice, "vkAllocateMemory"));
        next_vkFreeMemory = reinterpret_cast<PFN_vkFreeMemory>(
            next_vkGetDeviceProcAddr(*pDevice, "vkFreeMemory"));
        next_vkCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCreateSemaphore"));
        next_vkDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(
            next_vkGetDeviceProcAddr(*pDevice, "vkDestroySemaphore"));
        next_vkGetSemaphoreFdKHR = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkGetSemaphoreFdKHR"));
        next_vkGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(
            next_vkGetDeviceProcAddr(*pDevice, "vkGetDeviceQueue"));
        next_vkQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(
            next_vkGetDeviceProcAddr(*pDevice, "vkQueueSubmit"));
        next_vkCmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCmdPipelineBarrier"));
        next_vkCmdPipelineBarrier2 = reinterpret_cast<PFN_vkCmdPipelineBarrier2>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCmdPipelineBarrier2"));
        next_vkCmdCopyImage = reinterpret_cast<PFN_vkCmdCopyImage>(
            next_vkGetDeviceProcAddr(*pDevice, "vkCmdCopyImage"));
        next_vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(
            next_vkGetDeviceProcAddr(*pDevice, "vkAcquireNextImageKHR"));
        if (!next_vkDestroyDevice || !next_vkCreateSwapchainKHR ||
            !next_vkQueuePresentKHR || !next_vkDestroySwapchainKHR ||
            !next_vkGetSwapchainImagesKHR || !next_vkAllocateCommandBuffers ||
            !next_vkFreeCommandBuffers || !next_vkBeginCommandBuffer ||
            !next_vkEndCommandBuffer || !next_vkCreateCommandPool ||
            !next_vkDestroyCommandPool || !next_vkCreateImage ||
            !next_vkDestroyImage || !next_vkGetImageMemoryRequirements ||
            !next_vkBindImageMemory || !next_vkGetMemoryFdKHR ||
            !next_vkAllocateMemory || !next_vkFreeMemory ||
            !next_vkCreateSemaphore || !next_vkDestroySemaphore ||
            !next_vkGetSemaphoreFdKHR || !next_vkGetDeviceQueue ||
            !next_vkQueueSubmit || !next_vkCmdPipelineBarrier ||
            !next_vkCmdPipelineBarrier2 || !next_vkCmdCopyImage ||
            !next_vkAcquireNextImageKHR) {
            Log::error("lsfg-vk(layer): Failed to get device function pointers");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layer_vkCreateDevice2 = reinterpret_cast<PFN_vkCreateDevice>(
            Hooks::hooks["vkCreateDevicePost"]);
        res = layer_vkCreateDevice2(physicalDevice, pCreateInfo, pAllocator, pDevice);
        if (res != VK_SUCCESS) {
            Log::error("lsfg-vk(layer): Failed to create Vulkan device: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        Log::debug("lsfg-vk(layer): Successfully initialized lsfg-vk device layer");
        return res;
    } // NOLINTEND
}

const std::unordered_map<std::string, PFN_vkVoidFunction> layerFunctions = {
    { "vkCreateInstance",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateInstance) },
    { "vkGetInstanceProcAddr",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetInstanceProcAddr) },
    { "vkGetDeviceProcAddr",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkGetDeviceProcAddr) },
    { "vkCreateDevice",
        reinterpret_cast<PFN_vkVoidFunction>(&layer_vkCreateDevice) },
};

PFN_vkVoidFunction layer_vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    std::string name(pName);
    auto it = layerFunctions.find(name);
    if (it != layerFunctions.end()) {
        Log::debug("lsfg-vk(layer): Inserted layer function for {}", name);
        return it->second;
    }

    it = Hooks::hooks.find(name);
    if (it != Hooks::hooks.end()) {
        Log::debug("lsfg-vk(layer): Inserted hook function for {}", name);
        return it->second;
    }

    return next_vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction layer_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    std::string name(pName);
    auto it = layerFunctions.find(name);
    if (it != layerFunctions.end()) {
        Log::debug("lsfg-vk(layer): Inserted layer function for {}", name);
        return it->second;
    }

    it = Hooks::hooks.find(name);
    if (it != Hooks::hooks.end()) {
        Log::debug("lsfg-vk(layer): Inserted hook function for {}", name);
        return it->second;
    }

    return next_vkGetDeviceProcAddr(device, pName);
}

// original functions

VkResult Layer::ovkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {
    return next_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}
void Layer::ovkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator) {
    next_vkDestroyInstance(instance, pAllocator);
}

VkResult Layer::ovkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice) {
    return next_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}
void Layer::ovkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator) {
    next_vkDestroyDevice(device, pAllocator);
}

PFN_vkVoidFunction Layer::ovkGetInstanceProcAddr(
        VkInstance instance,
        const char* pName) {
    return next_vkGetInstanceProcAddr(instance, pName);
}
PFN_vkVoidFunction Layer::ovkGetDeviceProcAddr(
        VkDevice device,
        const char* pName) {
    return next_vkGetDeviceProcAddr(device, pName);
}

VkResult Layer::ovkCreateSwapchainKHR(
        VkDevice device,
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSwapchainKHR* pSwapchain) {
    return next_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
}
VkResult Layer::ovkQueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo) {
    return next_vkQueuePresentKHR(queue, pPresentInfo);
}
void Layer::ovkDestroySwapchainKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const VkAllocationCallbacks* pAllocator) {
    next_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult Layer::ovkGetSwapchainImagesKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint32_t* pSwapchainImageCount,
        VkImage* pSwapchainImages) {
    return next_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult Layer::ovkAllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers) {
    return next_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
}
void Layer::ovkFreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers) {
    next_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult Layer::ovkBeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo) {
    return next_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}
VkResult Layer::ovkEndCommandBuffer(
        VkCommandBuffer commandBuffer) {
    return next_vkEndCommandBuffer(commandBuffer);
}

VkResult Layer::ovkCreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool) {
    return next_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}
void Layer::ovkDestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator) {
    next_vkDestroyCommandPool(device, commandPool, pAllocator);
}

VkResult Layer::ovkCreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage) {
    return next_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
}
void Layer::ovkDestroyImage(
        VkDevice device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator) {
    next_vkDestroyImage(device, image, pAllocator);
}

void Layer::ovkGetImageMemoryRequirements(
        VkDevice device,
        VkImage image,
        VkMemoryRequirements* pMemoryRequirements) {
    next_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}
VkResult Layer::ovkBindImageMemory(
        VkDevice device,
        VkImage image,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset) {
    return next_vkBindImageMemory(device, image, memory, memoryOffset);
}

VkResult Layer::ovkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory) {
    return next_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}
void Layer::ovkFreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {
    next_vkFreeMemory(device, memory, pAllocator);
}

VkResult Layer::ovkCreateSemaphore(
        VkDevice device,
        const VkSemaphoreCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSemaphore* pSemaphore) {
    return next_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}
void Layer::ovkDestroySemaphore(
        VkDevice device,
        VkSemaphore semaphore,
        const VkAllocationCallbacks* pAllocator) {
    next_vkDestroySemaphore(device, semaphore, pAllocator);
}

VkResult Layer::ovkGetMemoryFdKHR(
        VkDevice device,
        const VkMemoryGetFdInfoKHR* pGetFdInfo,
        int* pFd) {
    return next_vkGetMemoryFdKHR(device, pGetFdInfo, pFd);
}
VkResult Layer::ovkGetSemaphoreFdKHR(
        VkDevice device,
        const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
        int* pFd) {
    return next_vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
}

void Layer::ovkGetPhysicalDeviceQueueFamilyProperties(
        VkPhysicalDevice physicalDevice,
        uint32_t* pQueueFamilyPropertyCount,
        VkQueueFamilyProperties* pQueueFamilyProperties) {
    next_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}
void Layer::ovkGetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    next_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

void Layer::ovkGetDeviceQueue(
        VkDevice device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex,
        VkQueue* pQueue) {
    next_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}
VkResult Layer::ovkQueueSubmit(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo* pSubmits,
        VkFence fence) {
    return next_vkQueueSubmit(queue, submitCount, pSubmits, fence);
}

void Layer::ovkCmdPipelineBarrier(
        VkCommandBuffer commandBuffer,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount,
        const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        const VkImageMemoryBarrier* pImageMemoryBarriers) {
    next_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
        memoryBarrierCount, pMemoryBarriers,
        bufferMemoryBarrierCount, pBufferMemoryBarriers,
        imageMemoryBarrierCount, pImageMemoryBarriers);
}
void Layer::ovkCmdPipelineBarrier2(
        VkCommandBuffer commandBuffer,
        const VkDependencyInfo* pDependencyInfo) {
    next_vkCmdPipelineBarrier2(commandBuffer, pDependencyInfo);
}
void Layer::ovkCmdCopyImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkImageCopy* pRegions) {
    next_vkCmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VkResult Layer::ovkAcquireNextImageKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint64_t timeout,
        VkSemaphore semaphore,
        VkFence fence,
        uint32_t* pImageIndex) {
    return next_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
}
