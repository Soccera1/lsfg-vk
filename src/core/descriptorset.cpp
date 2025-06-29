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

void DescriptorSet::update(const Device& device,
        const std::vector<std::vector<ResourcePair>>& resources) const {
    if (!device)
        throw std::invalid_argument("Invalid Vulkan device");

    std::vector<VkWriteDescriptorSet> writeDescriptorSets;
    uint32_t bindingIndex = 0;
    for (const auto& list : resources) {
        for (const auto& [type, resource] : list) {
            VkWriteDescriptorSet writeDesc{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = this->handle(),
                .dstBinding = bindingIndex++,
                .descriptorCount = 1,
                .descriptorType = type,
            };

            if (std::holds_alternative<Image>(resource)) {
                const auto& image = std::get<Image>(resource);
                if (!image)
                    throw std::invalid_argument("Invalid image resource");

                const VkDescriptorImageInfo imageInfo{
                    .imageView = image.getView(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL
                };
                writeDesc.pImageInfo = &imageInfo;
            } else if (std::holds_alternative<Sampler>(resource)) {
                const auto& sampler = std::get<Sampler>(resource);
                if (!sampler)
                    throw std::invalid_argument("Invalid sampler resource");

                const VkDescriptorImageInfo imageInfo{
                    .sampler = sampler.handle()
                };
                writeDesc.pImageInfo = &imageInfo;
            } else if (std::holds_alternative<Buffer>(resource)) {
                const auto& buffer = std::get<Buffer>(resource);
                if (!buffer)
                    throw std::invalid_argument("Invalid buffer resource");

                const VkDescriptorBufferInfo bufferInfo{
                    .buffer = buffer.handle(),
                    .range = buffer.getSize()
                };
                writeDesc.pBufferInfo = &bufferInfo;
            } else {
                throw std::invalid_argument("Unsupported resource type");
            }

            writeDescriptorSets.push_back(writeDesc);
        }
    }

    vkUpdateDescriptorSets(device.handle(),
        static_cast<uint32_t>(writeDescriptorSets.size()),
        writeDescriptorSets.data(),
        0, nullptr);
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
