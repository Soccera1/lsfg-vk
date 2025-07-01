#include "application.hpp"

#include <lsfg.hpp>

#include <stdexcept>

Application::Application(VkDevice device, VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue, VkQueue presentQueue)
        : device(device), physicalDevice(physicalDevice),
          graphicsQueue(graphicsQueue), presentQueue(presentQueue) {}

void Application::addSwapchain(VkSwapchainKHR handle, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images) {
    auto it = this->swapchains.find(handle);
    if (it != this->swapchains.end())
        return; // already added

    this->swapchains.emplace(handle, SwapchainContext(*this, handle, format, extent, images));
}

SwapchainContext::SwapchainContext(const Application& app, VkSwapchainKHR swapchain,
        VkFormat format, VkExtent2D extent, const std::vector<VkImage>& images)
        : swapchain(swapchain), format(format), extent(extent), images(images) {}

void Application::presentSwapchain(VkSwapchainKHR handle, VkQueue queue,
        const std::vector<VkSemaphore>& semaphores, uint32_t idx) {
    auto it = this->swapchains.find(handle);
    if (it == this->swapchains.end())
        throw std::logic_error("Swapchain not found");

    it->second.present(*this, queue, semaphores, idx);
}

void SwapchainContext::present(const Application& app, VkQueue queue,
        const std::vector<VkSemaphore>& semaphores, uint32_t idx) {
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<uint32_t>(semaphores.size()),
        .pWaitSemaphores = semaphores.data(),
        .swapchainCount = 1,
        .pSwapchains = &this->swapchain,
        .pImageIndices = &idx
    };
    auto res = vkQueuePresentKHR(queue, &presentInfo);
    if (res != VK_SUCCESS) // FIXME: somehow return VK_SUBOPTIMAL_KHR
        throw LSFG::vulkan_error(res, "Failed to present swapchain");
}

bool Application::removeSwapchain(VkSwapchainKHR handle) {
    auto it = this->swapchains.find(handle);
    if (it == this->swapchains.end())
        return false; // already retired... hopefully
    this->swapchains.erase(it);
    return true;
}
