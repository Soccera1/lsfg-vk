#include "application.hpp"
#include "mini/commandbuffer.hpp"
#include "mini/commandpool.hpp"
#include "mini/image.hpp"
#include "mini/semaphore.hpp"

#include <exception>
#include <iostream>
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

    int outfd{};
    this->out_img = Mini::Image(
        app.getDevice(), app.getPhysicalDevice(),
        extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, &outfd
    );

    auto id = LSFG::createContext(extent.width, extent.height, frame0fd, frame1fd, outfd);
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

    // present deferred frame if any
    if (this->deferredIdx.has_value()) {
        VkSemaphore deferredSemaphore = this->copySemaphores2.at((this->frameIdx - 1) % 8).handle();
        const uint32_t deferredIdx = this->deferredIdx.value();
        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &deferredSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &this->swapchain,
            .pImageIndices = &deferredIdx,
        };
        auto res = vkQueuePresentKHR(queue, &presentInfo);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            throw LSFG::vulkan_error(res, "Failed to present deferred swapchain image");
    }
    this->deferredIdx.emplace(idx);

    // create semaphores
    int copySem{};
    Mini::Semaphore& copySemaphore1 = this->copySemaphores1.at(this->frameIdx % 8);
    copySemaphore1 = Mini::Semaphore(app.getDevice(), &copySem);
    Mini::Semaphore& copySemaphore2 = this->copySemaphores2.at(this->frameIdx % 8);
    copySemaphore2 = Mini::Semaphore(app.getDevice());
    Mini::Semaphore& acquireSemaphore = this->acquireSemaphores.at(this->frameIdx % 8);
    acquireSemaphore = Mini::Semaphore(app.getDevice());
    int renderSem{};
    Mini::Semaphore& renderSemaphore = this->renderSemaphores.at(this->frameIdx % 8);
    renderSemaphore = Mini::Semaphore(app.getDevice(), &renderSem);
    Mini::Semaphore& presentSemaphore = this->presentSemaphores.at(this->frameIdx % 8);
    presentSemaphore = Mini::Semaphore(app.getDevice());

    // copy swapchain image to next frame image
    auto& cmdBuf1 = this->cmdBufs1.at(this->frameIdx % 8);
    cmdBuf1 = Mini::CommandBuffer(app.getDevice(), this->cmdPool);
    cmdBuf1.begin();

    auto& srcImage1 = this->images.at(idx);
    auto& dstImage1 = (this->frameIdx % 2 == 0) ? this->frame_0 : this->frame_1;

    const VkImageMemoryBarrier srcBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = srcImage1,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const VkImageMemoryBarrier dstBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = dstImage1.handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const std::vector<VkImageMemoryBarrier> barriers = { srcBarrier, dstBarrier };
    vkCmdPipelineBarrier(cmdBuf1.handle(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 2, barriers.data());

    const VkImageCopy imageCopy{
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .extent = {
            .width = this->extent.width,
            .height = this->extent.height,
            .depth = 1
        }
    };
    vkCmdCopyImage(cmdBuf1.handle(),
        srcImage1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage1.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imageCopy);

    const VkImageMemoryBarrier presentBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = dstImage1.handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vkCmdPipelineBarrier(cmdBuf1.handle(),
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

    cmdBuf1.end();
    cmdBuf1.submit(app.getGraphicsQueue(),
        semaphores, { copySemaphore1.handle(), copySemaphore2.handle() });

    // render the intermediary frame
    try {
        LSFG::presentContext(*this->lsfgId, copySem, renderSem);
    } catch(std::exception& e) {
        std::cerr << "LSFG error: " << e.what() << '\n';
    }

    // acquire the next swapchain image
    uint32_t newIdx{};
    auto res = vkAcquireNextImageKHR(app.getDevice(), this->swapchain, UINT64_MAX,
        acquireSemaphore.handle(), VK_NULL_HANDLE, &newIdx);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw LSFG::vulkan_error(res, "Failed to acquire next swapchain image");

    // copy the output image to the swapchain image
    auto& cmdBuf2 = this->cmdBufs2.at((this->frameIdx + 1) % 8);
    cmdBuf2 = Mini::CommandBuffer(app.getDevice(), this->cmdPool);
    cmdBuf2.begin();

    auto& srcImage2 = this->out_img;
    auto& dstImage2 = this->images.at(newIdx);

    const VkImageMemoryBarrier srcBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = srcImage2.handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const VkImageMemoryBarrier dstBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = dstImage2,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const std::vector<VkImageMemoryBarrier> barriers2 = { srcBarrier2, dstBarrier2 };
    vkCmdPipelineBarrier(cmdBuf2.handle(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 2, barriers2.data());

    const VkImageCopy imageCopy2{
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .extent = {
            .width = this->extent.width,
            .height = this->extent.height,
            .depth = 1
        }
    };
    vkCmdCopyImage(cmdBuf2.handle(),
        srcImage2.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstImage2, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imageCopy2);

    const VkImageMemoryBarrier presentBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = dstImage2,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vkCmdPipelineBarrier(cmdBuf2.handle(),
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &presentBarrier2);

    cmdBuf2.end();
    cmdBuf2.submit(app.getGraphicsQueue(),
        { acquireSemaphore.handle(), renderSemaphore.handle() },
        { presentSemaphore.handle() });

    // present the swapchain image
    VkSemaphore presentSemaphoreHandle = presentSemaphore.handle();
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentSemaphoreHandle,
        .swapchainCount = 1,
        .pSwapchains = &this->swapchain,
        .pImageIndices = &newIdx,
    };
    res = vkQueuePresentKHR(queue, &presentInfo);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) // FIXME: somehow return VK_SUBOPTIMAL_KHR
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
