#include "application.hpp"
#include "log.hpp"
#include "mini/image.hpp"

#include <lsfg.hpp>
#include <vulkan/vulkan_core.h>

Application::Application(VkDevice device, VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue, VkQueue presentQueue)
        : device(device), physicalDevice(physicalDevice),
          graphicsQueue(graphicsQueue), presentQueue(presentQueue) {
    LSFG::initialize();
}

void Application::addSwapchain(VkSwapchainKHR handle, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images) {
    auto it = this->swapchains.find(handle);
    if (it != this->swapchains.end())
        return; // already added

    this->swapchains.emplace(handle, SwapchainContext(*this, handle, format, extent, images));
}

SwapchainContext::SwapchainContext(const Application& app, VkSwapchainKHR swapchain,
        VkFormat format, VkExtent2D extent, const std::vector<VkImage>& images)
        : swapchain(swapchain), format(format), extent(extent), images(images) {
    int frame0fd{};
    this->frame_0 = Mini::Image(
        app.getDevice(), app.getPhysicalDevice(),
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, &frame0fd
    );
    int frame1fd{};
    this->frame_1 = Mini::Image(
        app.getDevice(), app.getPhysicalDevice(),
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, &frame1fd
    );
    this->lsfgId = LSFG::createContext(extent.width, extent.height, frame0fd, frame1fd);
}

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

SwapchainContext::~SwapchainContext() {
    try {
        LSFG::deleteContext(this->lsfgId);
    } catch (const std::exception&) {
        return;
    }
}

bool Application::removeSwapchain(VkSwapchainKHR handle) {
    auto it = this->swapchains.find(handle);
    if (it == this->swapchains.end())
        return false; // already retired... hopefully
    this->swapchains.erase(it);
    return true;
}

Application::~Application() {
    this->swapchains.clear();
    LSFG::finalize();
}
