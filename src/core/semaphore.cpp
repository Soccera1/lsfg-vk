#include "core/semaphore.hpp"
#include "utils.hpp"

using namespace Vulkan::Core;

Semaphore::Semaphore(const Device& device, std::optional<uint32_t> initial) {
    // create semaphore
    const VkSemaphoreTypeCreateInfo typeInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = initial.value_or(0)
    };
    const VkSemaphoreCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = initial.has_value() ? &typeInfo : nullptr,
    };
    VkSemaphore semaphoreHandle{};
    auto res = vkCreateSemaphore(device.handle(), &desc, nullptr, &semaphoreHandle);
    if (res != VK_SUCCESS || semaphoreHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to create semaphore");

    // store semaphore in shared ptr
    this->isTimeline = initial.has_value();
    this->semaphore = std::shared_ptr<VkSemaphore>(
        new VkSemaphore(semaphoreHandle),
        [dev = device.handle()](VkSemaphore* semaphoreHandle) {
            vkDestroySemaphore(dev, *semaphoreHandle, nullptr);
        }
    );
}

void Semaphore::signal(const Device& device, uint64_t value) const {
    if (!this->isTimeline)
        throw std::logic_error("Invalid timeline semaphore");

    const VkSemaphoreSignalInfo signalInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = this->handle(),
        .value = value
    };
    auto res = vkSignalSemaphore(device.handle(), &signalInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Unable to signal semaphore");
}

bool Semaphore::wait(const Device& device, uint64_t value, uint64_t timeout) const {
    if (!this->isTimeline)
        throw std::logic_error("Invalid timeline semaphore");

    VkSemaphore semaphore = this->handle();
    const VkSemaphoreWaitInfo waitInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &semaphore,
        .pValues = &value
    };
    auto res = vkWaitSemaphores(device.handle(), &waitInfo, timeout);
    if (res != VK_SUCCESS && res != VK_TIMEOUT)
        throw ls::vulkan_error(res, "Unable to wait for semaphore");

    return res == VK_SUCCESS;
}
