#include "core/commandpool.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

CommandPool::CommandPool(const Device& device) {
    if (!device)
        throw std::invalid_argument("Invalid Vulkan device");

    // create command pool
    const VkCommandPoolCreateInfo desc = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = device.getComputeFamilyIdx()
    };
    VkCommandPool commandPoolHandle{};
    auto res = vkCreateCommandPool(device.handle(), &desc, nullptr, &commandPoolHandle);
    if (res != VK_SUCCESS || commandPoolHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to create command pool");

    // store the command pool in a shared pointer
    this->commandPool = std::shared_ptr<VkCommandPool>(
        new VkCommandPool(commandPoolHandle),
        [dev = device.handle()](VkCommandPool* commandPoolHandle) {
            vkDestroyCommandPool(dev, *commandPoolHandle, nullptr);
        }
    );
}
