#include "core/shadermodule.hpp"
#include "utils/exceptions.hpp"

#include <fstream>
#include <vector>

using namespace Vulkan::Core;

ShaderModule::ShaderModule(const Device& device, const std::string& path) {
    if (!device)
        throw std::invalid_argument("Invalid Vulkan device");

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
        .codeSize = code.size() * sizeof(uint32_t),
        .pCode = reinterpret_cast<const uint32_t*>(data_ptr)
    };
    VkShaderModule shaderModuleHandle{};
    auto res = vkCreateShaderModule(device.handle(), &createInfo, nullptr, &shaderModuleHandle);
    if (res != VK_SUCCESS || !shaderModuleHandle)
        throw ls::vulkan_error(res, "Failed to create shader module");

    // store shader module in shared ptr
    this->shaderModule = std::shared_ptr<VkShaderModule>(
        new VkShaderModule(shaderModuleHandle),
        [dev = device.handle()](VkShaderModule* shaderModuleHandle) {
            vkDestroyShaderModule(dev, *shaderModuleHandle, nullptr);
        }
    );
}
