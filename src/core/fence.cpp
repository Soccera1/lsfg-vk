#include "core/fence.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

Fence::Fence(const Device& device) {
     // create fence
    const VkFenceCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    VkFence fenceHandle{};
    auto res = vkCreateFence(device.handle(), &desc, nullptr, &fenceHandle);
    if (res != VK_SUCCESS || fenceHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to create fence");

    // store fence in shared ptr
    this->fence = std::shared_ptr<VkFence>(
        new VkFence(fenceHandle),
        [dev = device.handle()](VkFence* fenceHandle) {
            vkDestroyFence(dev, *fenceHandle, nullptr);
        }
    );
}

void Fence::reset(const Device& device) const {
    VkFence fenceHandle = this->handle();
    auto res = vkResetFences(device.handle(), 1, &fenceHandle);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Unable to reset fence");
}

bool Fence::wait(const Device& device, uint64_t timeout) const {
    VkFence fenceHandle = this->handle();
    auto res = vkWaitForFences(device.handle(), 1, &fenceHandle, VK_TRUE, timeout);
    if (res != VK_SUCCESS && res != VK_TIMEOUT)
        throw ls::vulkan_error(res, "Unable to wait for fence");

    return res == VK_SUCCESS;
}
