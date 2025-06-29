#include "utils/barriers.hpp"

using namespace Barriers;

void Barriers::insertBarrier(
        const Vulkan::Core::CommandBuffer& buffer,
        std::vector<Vulkan::Core::Image> r2wImages,
        std::vector<Vulkan::Core::Image> w2rImages) {
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
