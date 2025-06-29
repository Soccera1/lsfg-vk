#include "core/commandbuffer.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

CommandBuffer::CommandBuffer(const Device& device, const CommandPool& pool) {
    if (!device || !pool)
        throw std::invalid_argument("Invalid Vulkan device or command pool");

    // create command buffer
    const VkCommandBufferAllocateInfo desc = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool.handle(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer commandBufferHandle{};
    auto res = vkAllocateCommandBuffers(device.handle(), &desc, &commandBufferHandle);
    if (res != VK_SUCCESS || commandBuffer == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to allocate command buffer");

    // store command buffer in shared ptr
    this->state = std::make_shared<CommandBufferState>(CommandBufferState::Empty);
    this->commandBuffer = std::shared_ptr<VkCommandBuffer>(
        new VkCommandBuffer(commandBufferHandle),
        [dev = device.handle(), pool = pool.handle()](VkCommandBuffer* cmdBuffer) {
            vkFreeCommandBuffers(dev, pool, 1, cmdBuffer);
        }
    );
}

void CommandBuffer::begin() {
    if (*this->state != CommandBufferState::Empty)
        throw std::logic_error("Command buffer is not in Empty state");

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    auto res = vkBeginCommandBuffer(*this->commandBuffer, &beginInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Unable to begin command buffer");

    *this->state = CommandBufferState::Recording;
}

void CommandBuffer::end() {
    if (*this->state != CommandBufferState::Recording)
        throw std::logic_error("Command buffer is not in Recording state");

    auto res = vkEndCommandBuffer(*this->commandBuffer);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Unable to end command buffer");

    *this->state = CommandBufferState::Full;
}

void CommandBuffer::submit(VkQueue queue, std::optional<Fence> fence,
        const std::vector<Semaphore>& waitSemaphores,
        std::optional<std::vector<uint64_t>> waitSemaphoreValues,
        const std::vector<Semaphore>& signalSemaphores,
        std::optional<std::vector<uint64_t>> signalSemaphoreValues) {
    if (!queue)
        throw std::invalid_argument("Invalid Vulkan queue");
    if (*this->state != CommandBufferState::Full)
        throw std::logic_error("Command buffer is not in Full state");

    const std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    VkTimelineSemaphoreSubmitInfo timelineInfo = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
    };
    if (waitSemaphoreValues.has_value()) {
        timelineInfo.waitSemaphoreValueCount =
            static_cast<uint32_t>(waitSemaphoreValues->size());
        timelineInfo.pWaitSemaphoreValues = waitSemaphoreValues->data();
    }
    if (signalSemaphoreValues.has_value()) {
        timelineInfo.signalSemaphoreValueCount =
            static_cast<uint32_t>(signalSemaphoreValues->size());
        timelineInfo.pSignalSemaphoreValues = signalSemaphoreValues->data();
    }

    std::vector<VkSemaphore> waitSemaphoresHandles;
    for (const auto& semaphore : waitSemaphores) {
        if (!semaphore)
            throw std::invalid_argument("Invalid Vulkan semaphore in waitSemaphores");
        waitSemaphoresHandles.push_back(semaphore.handle());
    }
    std::vector<VkSemaphore> signalSemaphoresHandles;
    for (const auto& semaphore : signalSemaphores) {
        if (!semaphore)
            throw std::invalid_argument("Invalid Vulkan semaphore in signalSemaphores");
        signalSemaphoresHandles.push_back(semaphore.handle());
    }

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = (waitSemaphoreValues.has_value() || signalSemaphoreValues.has_value())
            ? &timelineInfo : nullptr,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphoresHandles.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &(*this->commandBuffer),
        .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphores = signalSemaphoresHandles.data()
    };
    auto res = vkQueueSubmit(queue, 1, &submitInfo, fence ? fence->handle() : VK_NULL_HANDLE);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Unable to submit command buffer");

    *this->state = CommandBufferState::Submitted;
}
