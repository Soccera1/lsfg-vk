#include "core/descriptorset.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

DescriptorSet::DescriptorSet(const Device& device,
        DescriptorPool pool, const ShaderModule& shaderModule) {
    if (!device || !pool)
        throw std::invalid_argument("Invalid Vulkan device");

    // create descriptor set
    VkDescriptorSetLayout layout = shaderModule.getDescriptorSetLayout();
    const VkDescriptorSetAllocateInfo desc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool.handle(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout
    };
    VkDescriptorSet descriptorSetHandle{};
    auto res = vkAllocateDescriptorSets(device.handle(), &desc, &descriptorSetHandle);
    if (res != VK_SUCCESS || descriptorSetHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to allocate descriptor set");

    /// store descriptor set in shared ptr
    this->descriptorSet = std::shared_ptr<VkDescriptorSet>(
        new VkDescriptorSet(descriptorSetHandle),
        [dev = device.handle(), pool = std::move(pool)](VkDescriptorSet* setHandle) {
            vkFreeDescriptorSets(dev, pool.handle(), 1, setHandle);
        }
    );
}

void DescriptorSet::bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline) const {
    if (!commandBuffer)
        throw std::invalid_argument("Invalid command buffer");

    // bind descriptor set
    VkDescriptorSet descriptorSetHandle = this->handle();
    vkCmdBindDescriptorSets(commandBuffer.handle(),
        VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.getLayout(),
        0, 1, &descriptorSetHandle,
           0, nullptr);
}
