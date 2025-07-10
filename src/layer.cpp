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

    PFN_vkSetDeviceLoaderData next_vSetDeviceLoaderData{};

    PFN_vkGetInstanceProcAddr next_vkGetInstanceProcAddr{};
    PFN_vkGetDeviceProcAddr   next_vkGetDeviceProcAddr{};

    PFN_vkGetPhysicalDeviceQueueFamilyProperties next_vkGetPhysicalDeviceQueueFamilyProperties{};
    PFN_vkGetPhysicalDeviceMemoryProperties next_vkGetPhysicalDeviceMemoryProperties{};
    PFN_vkGetPhysicalDeviceProperties next_vkGetPhysicalDeviceProperties{};

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
    PFN_vkGetDeviceQueue next_vkGetDeviceQueue{};
    PFN_vkQueueSubmit next_vkQueueSubmit{};
    PFN_vkCmdPipelineBarrier next_vkCmdPipelineBarrier{};
    PFN_vkCmdBlitImage next_vkCmdBlitImage{};
    PFN_vkAcquireNextImageKHR next_vkAcquireNextImageKHR{};

    template<typename T>
    bool initInstanceFunc(VkInstance instance, const char* name, T* func) {
        *func = reinterpret_cast<T>(next_vkGetInstanceProcAddr(instance, name));
        if (!*func) {
            Log::error("layer", "Failed to get instance function pointer for {}", name);
            return false;
        }
        return true;
    }

    template<typename T>
    bool initDeviceFunc(VkDevice device, const char* name, T* func) {
        *func = reinterpret_cast<T>(next_vkGetDeviceProcAddr(device, name));
        if (!*func) {
            Log::error("layer", "Failed to get device function pointer for {}", name);
            return false;
        }
        return true;
    }
}


namespace {
    VkInstance gInstance;
    VkResult layer_vkCreateInstance( // NOLINTBEGIN
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        Log::debug("layer", "Initializing lsfg-vk instance layer...");

        // find layer creation info
        auto* layerDesc = const_cast<VkLayerInstanceCreateInfo*>(
            reinterpret_cast<const VkLayerInstanceCreateInfo*>(pCreateInfo->pNext));
        while (layerDesc && (layerDesc->sType != VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
                || layerDesc->function != VK_LAYER_LINK_INFO)) {
            layerDesc = const_cast<VkLayerInstanceCreateInfo*>(
                reinterpret_cast<const VkLayerInstanceCreateInfo*>(layerDesc->pNext));
        }
        if (!layerDesc) {
            Log::error("layer", "No layer creation info found in pNext chain");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // advance link info (i don't really know what this does)
        next_vkGetInstanceProcAddr = layerDesc->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        Log::debug("layer", "Next instance proc addr: {:x}",
            reinterpret_cast<uintptr_t>(next_vkGetInstanceProcAddr));

        layerDesc->u.pLayerInfo = layerDesc->u.pLayerInfo->pNext;

        // create instance
        auto success = initInstanceFunc(nullptr, "vkCreateInstance", &next_vkCreateInstance);
        if (!success) return VK_ERROR_INITIALIZATION_FAILED;

        auto* layer_vkCreateInstance2 = reinterpret_cast<PFN_vkCreateInstance>(
            Hooks::hooks["vkCreateInstance"]);
        auto res = layer_vkCreateInstance2(pCreateInfo, pAllocator, pInstance);
        if (res != VK_SUCCESS) {
            Log::error("layer", "Failed to create Vulkan instance: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        // get relevant function pointers from the next layer
        success = true;
        success &= initInstanceFunc(*pInstance, "vkDestroyInstance", &next_vkDestroyInstance);
        success &= initInstanceFunc(*pInstance,
            "vkGetPhysicalDeviceQueueFamilyProperties", &next_vkGetPhysicalDeviceQueueFamilyProperties);
        success &= initInstanceFunc(*pInstance,
            "vkGetPhysicalDeviceMemoryProperties", &next_vkGetPhysicalDeviceMemoryProperties);
        success &= initInstanceFunc(*pInstance,
            "vkGetPhysicalDeviceProperties", &next_vkGetPhysicalDeviceProperties);
        if (!success) {
            Log::error("layer", "Failed to get instance function pointers");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        gInstance = *pInstance; // workaround mesa bug

        Log::debug("layer", "Successfully initialized lsfg-vk instance layer");
        return res;
    } // NOLINTEND

    VkResult layer_vkCreateDevice( // NOLINTBEGIN
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        Log::debug("layer", "Initializing lsfg-vk device layer...");

        // find layer creation info
        auto* layerDesc = const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
        while (layerDesc && (layerDesc->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                || layerDesc->function != VK_LAYER_LINK_INFO)) {
            layerDesc = const_cast<VkLayerDeviceCreateInfo*>(
                reinterpret_cast<const VkLayerDeviceCreateInfo*>(layerDesc->pNext));
        }
        if (!layerDesc) {
            Log::error("layer", "No layer creation info found in pNext chain");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        next_vkGetDeviceProcAddr = layerDesc->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        Log::debug("layer", "Next device proc addr: {:x}",
            reinterpret_cast<uintptr_t>(next_vkGetDeviceProcAddr));

        // find second layer creation info
        auto* layerDesc2 = const_cast<VkLayerDeviceCreateInfo*>(
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext));
        while (layerDesc2 && (layerDesc2->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
                || layerDesc2->function != VK_LOADER_DATA_CALLBACK)) {
                    layerDesc2 = const_cast<VkLayerDeviceCreateInfo*>(
                        reinterpret_cast<const VkLayerDeviceCreateInfo*>(layerDesc2->pNext));
        }
        if (!layerDesc2) {
            Log::error("layer", "No layer creation info found in pNext chain");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        next_vSetDeviceLoaderData = layerDesc2->u.pfnSetDeviceLoaderData;
        Log::debug("layer", "Next device loader data: {:x}",
            reinterpret_cast<uintptr_t>(next_vSetDeviceLoaderData));

        // advance link info (i don't really know what this does)
        layerDesc->u.pLayerInfo = layerDesc->u.pLayerInfo->pNext;

        // create device
        auto success = initInstanceFunc(gInstance, "vkCreateDevice", &next_vkCreateDevice);
        if (!success) return VK_ERROR_INITIALIZATION_FAILED;

        auto* layer_vkCreateDevice2 = reinterpret_cast<PFN_vkCreateDevice>(
            Hooks::hooks["vkCreateDevicePre"]);
        auto res = layer_vkCreateDevice2(physicalDevice, pCreateInfo, pAllocator, pDevice);
        if (res != VK_SUCCESS) {
            Log::error("layer", "Failed to create Vulkan device: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        // get relevant function pointers from the next layer
        success = true;
        success &= initDeviceFunc(*pDevice, "vkDestroyDevice", &next_vkDestroyDevice);
        success &= initDeviceFunc(*pDevice, "vkCreateSwapchainKHR", &next_vkCreateSwapchainKHR);
        success &= initDeviceFunc(*pDevice, "vkQueuePresentKHR", &next_vkQueuePresentKHR);
        success &= initDeviceFunc(*pDevice, "vkDestroySwapchainKHR", &next_vkDestroySwapchainKHR);
        success &= initDeviceFunc(*pDevice, "vkGetSwapchainImagesKHR", &next_vkGetSwapchainImagesKHR);
        success &= initDeviceFunc(*pDevice, "vkAllocateCommandBuffers", &next_vkAllocateCommandBuffers);
        success &= initDeviceFunc(*pDevice, "vkFreeCommandBuffers", &next_vkFreeCommandBuffers);
        success &= initDeviceFunc(*pDevice, "vkBeginCommandBuffer", &next_vkBeginCommandBuffer);
        success &= initDeviceFunc(*pDevice, "vkEndCommandBuffer", &next_vkEndCommandBuffer);
        success &= initDeviceFunc(*pDevice, "vkCreateCommandPool", &next_vkCreateCommandPool);
        success &= initDeviceFunc(*pDevice, "vkDestroyCommandPool", &next_vkDestroyCommandPool);
        success &= initDeviceFunc(*pDevice, "vkCreateImage", &next_vkCreateImage);
        success &= initDeviceFunc(*pDevice, "vkDestroyImage", &next_vkDestroyImage);
        success &= initDeviceFunc(*pDevice, "vkGetImageMemoryRequirements", &next_vkGetImageMemoryRequirements);
        success &= initDeviceFunc(*pDevice, "vkBindImageMemory", &next_vkBindImageMemory);
        success &= initDeviceFunc(*pDevice, "vkGetMemoryFdKHR", &next_vkGetMemoryFdKHR);
        success &= initDeviceFunc(*pDevice, "vkAllocateMemory", &next_vkAllocateMemory);
        success &= initDeviceFunc(*pDevice, "vkFreeMemory", &next_vkFreeMemory);
        success &= initDeviceFunc(*pDevice, "vkCreateSemaphore", &next_vkCreateSemaphore);
        success &= initDeviceFunc(*pDevice, "vkDestroySemaphore", &next_vkDestroySemaphore);
        success &= initDeviceFunc(*pDevice, "vkGetSemaphoreFdKHR", &next_vkGetSemaphoreFdKHR);
        success &= initDeviceFunc(*pDevice, "vkGetDeviceQueue", &next_vkGetDeviceQueue);
        success &= initDeviceFunc(*pDevice, "vkQueueSubmit", &next_vkQueueSubmit);
        success &= initDeviceFunc(*pDevice, "vkCmdPipelineBarrier", &next_vkCmdPipelineBarrier);
        success &= initDeviceFunc(*pDevice, "vkCmdBlitImage", &next_vkCmdBlitImage);
        success &= initDeviceFunc(*pDevice, "vkAcquireNextImageKHR", &next_vkAcquireNextImageKHR);
        if (!success) {
            Log::error("layer", "Failed to get device function pointers");
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        layer_vkCreateDevice2 = reinterpret_cast<PFN_vkCreateDevice>(
            Hooks::hooks["vkCreateDevicePost"]);
        res = layer_vkCreateDevice2(physicalDevice, pCreateInfo, pAllocator, pDevice);
        if (res != VK_SUCCESS) {
            Log::error("layer", "Failed to create Vulkan device: {:x}",
                static_cast<uint32_t>(res));
            return res;
        }

        Log::debug("layer", "Successfully initialized lsfg-vk device layer");
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
    const std::string name(pName);
    auto it = layerFunctions.find(name);
    if (it != layerFunctions.end()) {
        Log::debug("layer", "Inserted layer function for {}", name);
        return it->second;
    }

    it = Hooks::hooks.find(name);
    if (it != Hooks::hooks.end()) {
        Log::debug("layer", "Inserted hook function for {}", name);
        return it->second;
    }

    return next_vkGetInstanceProcAddr(instance, pName);
}

PFN_vkVoidFunction layer_vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    const std::string name(pName);
    auto it = layerFunctions.find(name);
    if (it != layerFunctions.end()) {
        Log::debug("layer", "Inserted layer function for {}", name);
        return it->second;
    }

    it = Hooks::hooks.find(name);
    if (it != Hooks::hooks.end()) {
        Log::debug("layer", "Inserted hook function for {}", name);
        return it->second;
    }

    return next_vkGetDeviceProcAddr(device, pName);
}

// original functions

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
// NOLINTBEGIN

VkResult Layer::ovkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance) {
    Log::debug("vulkan", "vkCreateInstance called with {} extensions:",
        pCreateInfo->enabledExtensionCount);
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i)
        Log::debug("vulkan", "  - {}", pCreateInfo->ppEnabledExtensionNames[i]);
    auto res = next_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
    Log::debug("vulkan", "vkCreateInstance({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pInstance));
    return res;
}
void Layer::ovkDestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan", "vkDestroyInstance called for instance {:x}",
        reinterpret_cast<uintptr_t>(instance));
    next_vkDestroyInstance(instance, pAllocator);
}

VkResult Layer::ovkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice) {
    Log::debug("vulkan", "vkCreateDevice called with {} extensions:",
        pCreateInfo->enabledExtensionCount);
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; ++i)
        Log::debug("vulkan", "  - {}", pCreateInfo->ppEnabledExtensionNames[i]);
    auto res = next_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    Log::debug("vulkan", "vkCreateDevice({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pDevice));
    return res;
}
void Layer::ovkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan", "vkDestroyDevice called for device {:x}",
        reinterpret_cast<uintptr_t>(device));
    next_vkDestroyDevice(device, pAllocator);
    Log::debug("vulkan", "Device {:x} destroyed successfully",
        reinterpret_cast<uintptr_t>(device));
}

VkResult Layer::ovkSetDeviceLoaderData(VkDevice device, void* object) {
    Log::debug("vulkan", "vkSetDeviceLoaderData called for object {:x}",
        reinterpret_cast<uintptr_t>(object));
    return next_vSetDeviceLoaderData(device, object);
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

void Layer::ovkGetPhysicalDeviceQueueFamilyProperties(
        VkPhysicalDevice physicalDevice,
        uint32_t* pQueueFamilyPropertyCount,
        VkQueueFamilyProperties* pQueueFamilyProperties) {
    Log::debug("vulkan", "vkGetPhysicalDeviceQueueFamilyProperties called for physical device {:x}",
        reinterpret_cast<uintptr_t>(physicalDevice));
    next_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}
void Layer::ovkGetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties) {
    Log::debug("vulkan", "vkGetPhysicalDeviceMemoryProperties called for physical device {:x}",
        reinterpret_cast<uintptr_t>(physicalDevice));
    next_vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}
void Layer::ovkGetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties) {
    Log::debug("vulkan", "vkGetPhysicalDeviceProperties called for physical device {:x}",
        reinterpret_cast<uintptr_t>(physicalDevice));
    next_vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VkResult Layer::ovkCreateSwapchainKHR(
        VkDevice device,
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSwapchainKHR* pSwapchain) {
    Log::debug("vulkan", "vkCreateSwapchainKHR called with {} images, extent: {}x{}",
        pCreateInfo->minImageCount, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height);
    auto res = next_vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    Log::debug("vulkan", "vkCreateSwapchainKHR({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pSwapchain));
    return res;
}
VkResult Layer::ovkQueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo) {
    Log::debug("vulkan2", "vkQueuePresentKHR called with {} wait semaphores:",
        pPresentInfo->waitSemaphoreCount);
    for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; i++)
        Log::debug("vulkan2", "  - {:x}", reinterpret_cast<uintptr_t>(pPresentInfo->pWaitSemaphores[i]));
    Log::debug("vulkan2", "and {} signal semaphores:",
        pPresentInfo->swapchainCount);
    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++)
        Log::debug("vulkan2", "  - {:x}", reinterpret_cast<uintptr_t>(pPresentInfo->pSwapchains[i]));
    Log::debug("vulkan2", "and queue: {:x}, image: {}",
        reinterpret_cast<uintptr_t>(queue),
        *pPresentInfo->pImageIndices);
    auto res = next_vkQueuePresentKHR(queue, pPresentInfo);
    Log::debug("vulkan2", "vkQueuePresentKHR({}) returned",
        static_cast<uint32_t>(res));
    return res;
}
void Layer::ovkDestroySwapchainKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan", "vkDestroySwapchainKHR called for swapchain {:x}",
        reinterpret_cast<uintptr_t>(swapchain));
    next_vkDestroySwapchainKHR(device, swapchain, pAllocator);
}

VkResult Layer::ovkGetSwapchainImagesKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint32_t* pSwapchainImageCount,
        VkImage* pSwapchainImages) {
    Log::debug("vulkan", "vkGetSwapchainImagesKHR called for swapchain {:x}",
        reinterpret_cast<uintptr_t>(swapchain));
    auto res = next_vkGetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    Log::debug("vulkan", "vkGetSwapchainImagesKHR({}) returned {} images",
        static_cast<uint32_t>(res),
        *pSwapchainImageCount);
    return res;
}

VkResult Layer::ovkAllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers) {
    Log::debug("vulkan2", "vkAllocateCommandBuffers called for command pool {:x}",
        reinterpret_cast<uintptr_t>(pAllocateInfo->commandPool));
    auto res = next_vkAllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    Log::debug("vulkan2", "vkAllocateCommandBuffers({}) returned command buffer: {}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pCommandBuffers));
    return res;
}
void Layer::ovkFreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers) {
    Log::debug("vulkan2", "vkFreeCommandBuffers called for command buffer: {:x}",
        reinterpret_cast<uintptr_t>(*pCommandBuffers));
    next_vkFreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult Layer::ovkBeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo) {
    Log::debug("vulkan2", "vkBeginCommandBuffer called for command buffer {:x}",
        reinterpret_cast<uintptr_t>(commandBuffer));
    return next_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
}
VkResult Layer::ovkEndCommandBuffer(
        VkCommandBuffer commandBuffer) {
    Log::debug("vulkan2", "vkEndCommandBuffer called for command buffer {:x}",
        reinterpret_cast<uintptr_t>(commandBuffer));
    return next_vkEndCommandBuffer(commandBuffer);
}

VkResult Layer::ovkCreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool) {
    Log::debug("vulkan", "vkCreateCommandPool called");
    auto res = next_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    Log::debug("vulkan", "vkCreateCommandPool({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pCommandPool));
    return res;
}
void Layer::ovkDestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan", "vkDestroyCommandPool called for command pool {:x}",
        reinterpret_cast<uintptr_t>(commandPool));
    next_vkDestroyCommandPool(device, commandPool, pAllocator);
}

VkResult Layer::ovkCreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage) {
    Log::debug("vulkan", "vkCreateImage called with format: {}, extent: {}x{}, usage: {}",
        static_cast<uint32_t>(pCreateInfo->format),
        pCreateInfo->extent.width, pCreateInfo->extent.height,
        static_cast<uint32_t>(pCreateInfo->usage));
    auto res = next_vkCreateImage(device, pCreateInfo, pAllocator, pImage);
    Log::debug("vulkan", "vkCreateImage({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pImage));
    return res;
}
void Layer::ovkDestroyImage(
        VkDevice device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan", "vkDestroyImage called for image {:x}",
        reinterpret_cast<uintptr_t>(image));
    next_vkDestroyImage(device, image, pAllocator);
}

void Layer::ovkGetImageMemoryRequirements(
        VkDevice device,
        VkImage image,
        VkMemoryRequirements* pMemoryRequirements) {
    Log::debug("vulkan", "vkGetImageMemoryRequirements called for image {:x}",
        reinterpret_cast<uintptr_t>(image));
    next_vkGetImageMemoryRequirements(device, image, pMemoryRequirements);
}
VkResult Layer::ovkBindImageMemory(
        VkDevice device,
        VkImage image,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset) {
    Log::debug("vulkan", "vkBindImageMemory called for image {:x}, memory {:x}, offset: {}",
        reinterpret_cast<uintptr_t>(image),
        reinterpret_cast<uintptr_t>(memory),
        memoryOffset);
    auto res = next_vkBindImageMemory(device, image, memory, memoryOffset);
    Log::debug("vulkan", "vkBindImageMemory({}) returned",
        static_cast<uint32_t>(res));
    return res;
}

VkResult Layer::ovkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory) {
    Log::debug("vulkan", "vkAllocateMemory called with size: {}, memory type index: {}",
        pAllocateInfo->allocationSize,
        pAllocateInfo->memoryTypeIndex);
    auto res = next_vkAllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
    Log::debug("vulkan", "vkAllocateMemory({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pMemory));
    return res;
}
void Layer::ovkFreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan", "vkFreeMemory called for memory {:x}",
        reinterpret_cast<uintptr_t>(memory));
    next_vkFreeMemory(device, memory, pAllocator);
}

VkResult Layer::ovkCreateSemaphore(
        VkDevice device,
        const VkSemaphoreCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSemaphore* pSemaphore) {
    Log::debug("vulkan2", "vkCreateSemaphore called",
        static_cast<uint32_t>(pCreateInfo->flags));
    auto res = next_vkCreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    Log::debug("vulkan2", "vkCreateSemaphore({}) returned handle {:x}",
        static_cast<uint32_t>(res),
        reinterpret_cast<uintptr_t>(*pSemaphore));
    return res;
}
void Layer::ovkDestroySemaphore(
        VkDevice device,
        VkSemaphore semaphore,
        const VkAllocationCallbacks* pAllocator) {
    Log::debug("vulkan2", "vkDestroySemaphore called for semaphore {:x}",
        reinterpret_cast<uintptr_t>(semaphore));
    next_vkDestroySemaphore(device, semaphore, pAllocator);
}

VkResult Layer::ovkGetMemoryFdKHR(
        VkDevice device,
        const VkMemoryGetFdInfoKHR* pGetFdInfo,
        int* pFd) {
    Log::debug("vulkan", "vkGetMemoryFdKHR called for memory {:x}, handle type: {}",
        reinterpret_cast<uintptr_t>(pGetFdInfo->memory),
        static_cast<uint32_t>(pGetFdInfo->handleType));
    auto res = next_vkGetMemoryFdKHR(device, pGetFdInfo, pFd);
    Log::debug("vulkan", "vkGetMemoryFdKHR({}) returned fd: {}",
        static_cast<uint32_t>(res), *pFd);
    return res;
}
VkResult Layer::ovkGetSemaphoreFdKHR(
        VkDevice device,
        const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
        int* pFd) {
    Log::debug("vulkan2", "vkGetSemaphoreFdKHR called for semaphore {:x}",
        reinterpret_cast<uintptr_t>(pGetFdInfo->semaphore));
    auto res = next_vkGetSemaphoreFdKHR(device, pGetFdInfo, pFd);
    Log::debug("vulkan2", "vkGetSemaphoreFdKHR({}) returned fd: {}",
        static_cast<uint32_t>(res), *pFd);
    return res;
}

void Layer::ovkGetDeviceQueue(
        VkDevice device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex,
        VkQueue* pQueue) {
    Log::debug("vulkan", "vkGetDeviceQueue called for device {:x}, queue family index: {}, queue index: {}",
        reinterpret_cast<uintptr_t>(device),
        queueFamilyIndex,
        queueIndex);
    next_vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}
VkResult Layer::ovkQueueSubmit(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo* pSubmits,
        VkFence fence) {
    Log::debug("vulkan2", "vkQueueSubmit called for queue {:x}, submitting: {} with wait semaphores:",
        reinterpret_cast<uintptr_t>(queue),
        reinterpret_cast<uintptr_t>(*pSubmits->pCommandBuffers));
    for (uint32_t i = 0; i < pSubmits->waitSemaphoreCount; ++i)
        Log::debug("vulkan2", "  - {:x}", reinterpret_cast<uintptr_t>(pSubmits->pWaitSemaphores[i]));
    Log::debug("vulkan2", "and {} signal semaphores:",
        pSubmits->waitSemaphoreCount);
    for (uint32_t i = 0; i < submitCount; ++i)
        Log::debug("vulkan2", "  - {:x}", reinterpret_cast<uintptr_t>(pSubmits[i].pSignalSemaphores));
    Log::debug("vulkan2", "and fence: {:x}",
        reinterpret_cast<uintptr_t>(fence));
    auto res = next_vkQueueSubmit(queue, submitCount, pSubmits, fence);
    Log::debug("vulkan2", "vkQueueSubmit({}) returned",
        static_cast<uint32_t>(res));
    return res;
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
    Log::debug("vulkan2", "vkCmdPipelineBarrier called for command buffer {:x}, src stage: {}, dst stage: {}, transitioning:",
        reinterpret_cast<uintptr_t>(commandBuffer),
        static_cast<uint32_t>(srcStageMask),
        static_cast<uint32_t>(dstStageMask));
    for (uint32_t i = 0; i < imageMemoryBarrierCount; ++i) {
        Log::debug("vulkan2", "  - image {:x}, old layout: {}, new layout: {}",
            reinterpret_cast<uintptr_t>(pImageMemoryBarriers[i].image),
            static_cast<uint32_t>(pImageMemoryBarriers[i].oldLayout),
            static_cast<uint32_t>(pImageMemoryBarriers[i].newLayout));
    }
    next_vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
        memoryBarrierCount, pMemoryBarriers,
        bufferMemoryBarrierCount, pBufferMemoryBarriers,
        imageMemoryBarrierCount, pImageMemoryBarriers);
}
void Layer::ovkCmdBlitImage(
        VkCommandBuffer commandBuffer,
        VkImage srcImage,
        VkImageLayout srcImageLayout,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkImageBlit* pRegions,
        VkFilter filter) {
    Log::debug("vulkan2", "vkCmdBlitImage called for command buffer {:x}, src image {:x}, dst image {:x}",
        reinterpret_cast<uintptr_t>(commandBuffer),
        reinterpret_cast<uintptr_t>(srcImage),
        reinterpret_cast<uintptr_t>(dstImage));
    next_vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

VkResult Layer::ovkAcquireNextImageKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        uint64_t timeout,
        VkSemaphore semaphore,
        VkFence fence,
        uint32_t* pImageIndex) {
    Log::debug("vulkan", "vkAcquireNextImageKHR called for swapchain {:x}, timeout: {}, semaphore: {:x}, fence: {:x}",
        reinterpret_cast<uintptr_t>(swapchain),
        timeout,
        reinterpret_cast<uintptr_t>(semaphore),
        reinterpret_cast<uintptr_t>(fence));
    auto res = next_vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
    Log::debug("vulkan", "vkAcquireNextImageKHR({}) returned image index: {}",
        static_cast<uint32_t>(res),
        *pImageIndex);
    return res;
}

#pragma clang diagnostic pop
// NOLINTEND
