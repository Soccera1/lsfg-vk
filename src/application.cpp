#include "application.hpp"
#include "mini/commandpool.hpp"
#include "mini/image.hpp"
#include "mini/semaphore.hpp"

#include <lsfg.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>

Application::Application(VkDevice device, VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue, uint32_t graphicsQueueFamilyIndex)
        : device(device), physicalDevice(physicalDevice),
          graphicsQueue(graphicsQueue), graphicsQueueFamilyIndex(graphicsQueueFamilyIndex) {
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
    this->cmdPool = Mini::CommandPool(app.getDevice(), app.getGraphicsQueueFamilyIndex());

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

    auto id = LSFG::createContext(extent.width, extent.height, frame0fd, frame1fd);
    this->lsfgId = std::shared_ptr<int32_t>(
        new int32_t(id),
        [](const int32_t* id) {
            LSFG::deleteContext(*id);
        }
    );
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
    // create semaphores
    int copySem{};
    int presentSem{};
    Mini::Semaphore& copySemaphore = this->copySemaphores.at(this->frameIdx % 8);
    copySemaphore = Mini::Semaphore(app.getDevice(), &copySem);
    Mini::Semaphore& acquireSemaphore = this->acquireSemaphores.at(this->frameIdx % 8);
    acquireSemaphore = Mini::Semaphore(app.getDevice());
    Mini::Semaphore& renderSemaphore = this->renderSemaphores.at(this->frameIdx % 8);
    renderSemaphore = Mini::Semaphore(app.getDevice());
    Mini::Semaphore& presentSemaphore = this->presentSemaphores.at(this->frameIdx % 8);
    presentSemaphore = Mini::Semaphore(app.getDevice(), &presentSem);

    // ...

    // render the intermediary frame
    // FIXME: i forgot the flippin output
    // FIXME: semaphores are being destroyed in the context
    LSFG::presentContext(*this->lsfgId, copySem, presentSem);

    // ...

    // ...

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

    this->frameIdx++;
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
