#include "core/fence.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

Fence::Fence(const Device& device) {
    if (!device)
        throw std::invalid_argument("Invalid Vulkan device");

    // create fence
    const VkFenceCreateInfo desc = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    VkFence fenceHandle{};
    auto res = vkCreateFence(device.handle(), &desc, nullptr, &fenceHandle);
    if (res != VK_SUCCESS || fenceHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to create fence");

    // store fence in shared ptr
    this->device = device.handle();
    this->fence = std::shared_ptr<VkFence>(
        new VkFence(fenceHandle),
        [dev = device.handle()](VkFence* fenceHandle) {
            vkDestroyFence(dev, *fenceHandle, nullptr);
        }
    );
}

void Fence::reset() const {
    VkFence fenceHandle = this->handle();
    auto res = vkResetFences(this->device, 1, &fenceHandle);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Unable to reset fence");
}

bool Fence::wait(uint64_t timeout) const {
    VkFence fenceHandle = this->handle();
    auto res = vkWaitForFences(this->device, 1, &fenceHandle, VK_TRUE, timeout);
    if (res != VK_SUCCESS && res != VK_TIMEOUT)
        throw ls::vulkan_error(res, "Unable to wait for fence");

    return res == VK_SUCCESS;
}
