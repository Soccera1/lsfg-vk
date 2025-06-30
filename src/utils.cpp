#include "utils.hpp"
#include "core/buffer.hpp"

#include <format>
#include <fstream>

using namespace Vulkan;

void Utils::insertBarrier(
        const Core::CommandBuffer& buffer,
        std::vector<Core::Image> r2wImages,
        std::vector<Core::Image> w2rImages) {
    std::vector<VkImageMemoryBarrier2> barriers(r2wImages.size() + w2rImages.size());

    size_t index = 0;
    for (auto& image : r2wImages) {
        barriers[index++] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = image.getLayout(),
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = image.handle(),
            .subresourceRange = {
                .aspectMask = image.getAspectFlags(),
                .levelCount = 1,
                .layerCount = 1
            }
        };
        image.setLayout(VK_IMAGE_LAYOUT_GENERAL);
    }
    for (auto& image : w2rImages) {
        barriers[index++] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = image.getLayout(),
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = image.handle(),
            .subresourceRange = {
                .aspectMask = image.getAspectFlags(),
                .levelCount = 1,
                .layerCount = 1
            }
        };
        image.setLayout(VK_IMAGE_LAYOUT_GENERAL);
    }
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    };
    vkCmdPipelineBarrier2(buffer.handle(), &dependencyInfo);
}

void Utils::uploadImage(const Device& device, const Core::CommandPool& commandPool,
        Core::Image& image, const std::string& path) {
    // read image bytecode
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file)
        throw std::system_error(errno, std::generic_category(), "Failed to open image: " + path);

    std::streamsize size = file.tellg();
    size -= 124 - 4; // dds header and magic bytes
    std::vector<uint8_t> code(static_cast<size_t>(size));

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(code.data()), size))
        throw std::system_error(errno, std::generic_category(), "Failed to read image: " + path);

    file.close();

    // copy data to buffer
    const Core::Buffer stagingBuffer(
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

    // wait for the upload to complete
    if (!fence.wait(device))
        throw ls::vulkan_error(VK_TIMEOUT, "Upload operation timed out");
}

Core::Sampler Globals::samplerClampBorder;
Core::Sampler Globals::samplerClampEdge;
Globals::FgBuffer Globals::fgBuffer;

void Globals::initializeGlobals(const Device& device) {
    // initialize global samplers
    samplerClampBorder = Core::Sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    samplerClampEdge =   Core::Sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    // initialize global constant buffer
    fgBuffer = {
        .inputOffset = { 0, 29 },
        .resolutionInvScale = 1.0F,
        .timestamp = 0.5F,
        .uiThreshold = 0.1F,
    };
}

void Globals::uninitializeGlobals() noexcept {
    // uninitialize global samplers
    samplerClampBorder = Core::Sampler();
    samplerClampEdge = Core::Sampler();

    // uninitialize global constant buffer
    fgBuffer = Globals::FgBuffer();
}

ls::vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<uint32_t>(result))), result(result) {}

ls::vulkan_error::~vulkan_error() noexcept = default;
