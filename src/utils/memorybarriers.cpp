#include "utils/memorybarriers.hpp"

#include <optional>

using namespace Barriers;

void Barriers::insertBarrier(
        const Vulkan::Core::CommandBuffer& buffer,
        const std::vector<Vulkan::Core::Image>& r2wImages,
        const std::vector<Vulkan::Core::Image>& w2rImages,
        std::optional<VkImageLayout> srcLayout) {
    std::vector<VkImageMemoryBarrier2> barriers(r2wImages.size() + w2rImages.size());

    size_t index = 0;
    for (const auto& image : r2wImages) {
        barriers[index++] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .oldLayout = srcLayout.value_or(VK_IMAGE_LAYOUT_GENERAL),
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = image.handle(),
            .subresourceRange = {
                .aspectMask = image.getAspectFlags(),
                .levelCount = 1,
                .layerCount = 1
            }
        };
    }
    for (const auto& image : w2rImages) {
        barriers[index++] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
            .oldLayout = srcLayout.value_or(VK_IMAGE_LAYOUT_GENERAL),
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .image = image.handle(),
            .subresourceRange = {
                .aspectMask = image.getAspectFlags(),
                .levelCount = 1,
                .layerCount = 1
            }
        };
    }
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size()),
        .pImageMemoryBarriers = barriers.data()
    };
    vkCmdPipelineBarrier2(buffer.handle(), &dependencyInfo);
}

void Barriers::insertGlobalBarrier(const Vulkan::Core::CommandBuffer& buffer) {
    const VkMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT
    };
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrier
    };
    vkCmdPipelineBarrier2(buffer.handle(), &dependencyInfo);
}
