#include "core/pipeline.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

Pipeline::Pipeline(const Device& device, const ShaderModule& shader) {
    if (!device || !shader)
        throw std::invalid_argument("Invalid Vulkan device or shader module");

    // create pipeline layout
    VkDescriptorSetLayout shaderLayout = shader.getDescriptorSetLayout();
    const VkPipelineLayoutCreateInfo layoutDesc{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &shaderLayout,
    };
    VkPipelineLayout layoutHandle{};
    auto res = vkCreatePipelineLayout(device.handle(), &layoutDesc, nullptr, &layoutHandle);
    if (res != VK_SUCCESS || !layoutHandle)
        throw ls::vulkan_error(res, "Failed to create pipeline layout");

    // store layout in shared ptr
    this->layout = std::shared_ptr<VkPipelineLayout>(
        new VkPipelineLayout(layoutHandle),
        [dev = device.handle()](VkPipelineLayout* layout) {
            vkDestroyPipelineLayout(dev, *layout, nullptr);
        }
    );

    // create pipeline
    const VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader.handle(),
        .pName = "main",
    };
    const VkComputePipelineCreateInfo pipelineDesc{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shaderStageInfo,
        .layout = layoutHandle,
    };
    VkPipeline pipelineHandle{};
    res = vkCreateComputePipelines(device.handle(),
        VK_NULL_HANDLE, 1, &pipelineDesc, nullptr, &pipelineHandle);
    if (res != VK_SUCCESS || !pipelineHandle)
        throw ls::vulkan_error(res, "Failed to create compute pipeline");

    // store pipeline in shared ptr
    this->pipeline = std::shared_ptr<VkPipeline>(
        new VkPipeline(pipelineHandle),
        [dev = device.handle()](VkPipeline* pipeline) {
            vkDestroyPipeline(dev, *pipeline, nullptr);
        }
    );
}

void Pipeline::bind(const CommandBuffer& commandBuffer) const {
    if (!commandBuffer)
        throw std::invalid_argument("Invalid command buffer");

    vkCmdBindPipeline(commandBuffer.handle(), VK_PIPELINE_BIND_POINT_COMPUTE, *this->pipeline);
}
