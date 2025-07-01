#include "core/shadermodule.hpp"
#include "lsfg.hpp"

#include <fstream>

using namespace LSFG::Core;

ShaderModule::ShaderModule(const Device& device, const std::string& path,
        const std::vector<std::pair<size_t, VkDescriptorType>>& descriptorTypes) {
    // read shader bytecode
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file)
        throw std::system_error(errno, std::generic_category(), "Failed to open shader file: " + path);

    const std::streamsize size = file.tellg();
    std::vector<uint8_t> code(static_cast<size_t>(size));

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(code.data()), size))
        throw std::system_error(errno, std::generic_category(), "Failed to read shader file: " + path);

    file.close();

    // create shader module
    const uint8_t* data_ptr = code.data();
    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(data_ptr)
    };
    VkShaderModule shaderModuleHandle{};
    auto res = vkCreateShaderModule(device.handle(), &createInfo, nullptr, &shaderModuleHandle);
    if (res != VK_SUCCESS || !shaderModuleHandle)
        throw LSFG::vulkan_error(res, "Failed to create shader module");

    // create descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    size_t bindIdx = 0;
    for (const auto &[count, type] : descriptorTypes)
        for (size_t i = 0; i < count; i++, bindIdx++)
            layoutBindings.emplace_back(VkDescriptorSetLayoutBinding {
                .binding = static_cast<uint32_t>(bindIdx),
                .descriptorType = type,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            });

    const VkDescriptorSetLayoutCreateInfo layoutDesc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data()
    };
    VkDescriptorSetLayout descriptorSetLayout{};
    res = vkCreateDescriptorSetLayout(device.handle(), &layoutDesc, nullptr, &descriptorSetLayout);
    if (res != VK_SUCCESS || !descriptorSetLayout)
        throw LSFG::vulkan_error(res, "Failed to create descriptor set layout");

    // store module and layout in shared ptr
    this->shaderModule = std::shared_ptr<VkShaderModule>(
        new VkShaderModule(shaderModuleHandle),
        [dev = device.handle()](VkShaderModule* shaderModuleHandle) {
            vkDestroyShaderModule(dev, *shaderModuleHandle, nullptr);
        }
    );
    this->descriptorSetLayout = std::shared_ptr<VkDescriptorSetLayout>(
        new VkDescriptorSetLayout(descriptorSetLayout),
        [dev = device.handle()](VkDescriptorSetLayout* layout) {
            vkDestroyDescriptorSetLayout(dev, *layout, nullptr);
        }
    );
}
