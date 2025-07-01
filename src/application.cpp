#include "application.hpp"

#include <stdexcept>

Application::Application(VkDevice device, VkPhysicalDevice physicalDevice)
        : device(device), physicalDevice(physicalDevice) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE)
        throw std::invalid_argument("Invalid Vulkan device or physical device");
}

void Application::addSwapchain(VkSwapchainKHR handle, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images) {
    // add the swapchain handle
    auto it = this->swapchains.find(handle);
    if (it != this->swapchains.end())
        throw std::invalid_argument("Swapchain handle already exists");

    // initialize the swapchain context
    this->swapchains.emplace(handle, SwapchainContext(handle, format, extent, images));
}

SwapchainContext::SwapchainContext(VkSwapchainKHR swapchain, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images)
        : swapchain(swapchain), format(format), extent(extent), images(images) {
    if (swapchain == VK_NULL_HANDLE || format == VK_FORMAT_UNDEFINED)
        throw std::invalid_argument("Invalid swapchain or images");
}

bool Application::removeSwapchain(VkSwapchainKHR handle) {
    if (handle == VK_NULL_HANDLE)
        throw std::invalid_argument("Invalid swapchain handle");

    // remove the swapchain context
    auto it = this->swapchains.find(handle);
    if (it == this->swapchains.end())
        return false; // already retired... hopefully
    this->swapchains.erase(it);
    return true;
}
