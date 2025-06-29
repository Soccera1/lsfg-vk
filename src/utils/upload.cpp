#include "utils/upload.hpp"
#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "utils/exceptions.hpp"

#include <fstream>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace Upload;
using namespace Vulkan;

void Upload::upload(const Device& device, const Core::CommandPool& commandPool,
        Core::Image& image, const std::string& path) {
    // read iamge bytecode
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file)
        throw std::system_error(errno, std::generic_category(), "Failed to open image: " + path);

    std::streamsize size = file.tellg();
    size -= 124 - 4;
    std::vector<uint8_t> code(static_cast<size_t>(size));

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(code.data()), size))
        throw std::system_error(errno, std::generic_category(), "Failed to read image: " + path);

    file.close();

    // copy data to buffer
    const Vulkan::Core::Buffer stagingBuffer(
        device, code.data(), static_cast<uint32_t>(code.size()),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );

    // perform the upload
    Core::CommandBuffer commandBuffer(device, commandPool);
    commandBuffer.begin();

    const VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = image.getLayout(),
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = image.handle(),
        .subresourceRange = {
            .aspectMask = image.getAspectFlags(),
            .levelCount = 1,
            .layerCount = 1
        }
    };
    image.setLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdPipelineBarrier(
        commandBuffer.handle(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier
    );

    auto extent = image.getExtent();
    const VkBufferImageCopy region{
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = image.getAspectFlags(),
            .layerCount = 1
        },
        .imageExtent = { extent.width, extent.height, 1 }
    };
    vkCmdCopyBufferToImage(
        commandBuffer.handle(),
        stagingBuffer.handle(), image.handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
    );

    commandBuffer.end();

    Core::Fence fence(device);
    commandBuffer.submit(device.getComputeQueue(), fence);

    if (!fence.wait(device))
        throw ls::vulkan_error(VK_TIMEOUT, "Upload operation timed out");
}
