#include "mini/commandpool.hpp"
#include "layer.hpp"

#include <lsfg.hpp>

using namespace Mini;

CommandPool::CommandPool(VkDevice device, uint32_t graphicsFamilyIdx) {
    // create command pool
    const VkCommandPoolCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphicsFamilyIdx
    };
    VkCommandPool commandPoolHandle{};
    auto res = Layer::ovkCreateCommandPool(device, &desc, nullptr, &commandPoolHandle);
    if (res != VK_SUCCESS || commandPoolHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to create command pool");

    // store command pool in shared ptr
    this->commandPool = std::shared_ptr<VkCommandPool>(
        new VkCommandPool(commandPoolHandle),
        [dev = device](VkCommandPool* commandPoolHandle) {
            Layer::ovkDestroyCommandPool(dev, *commandPoolHandle, nullptr);
        }
    );
}
