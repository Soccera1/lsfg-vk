#include "core/buffer.hpp"
#include "utils/exceptions.hpp"

#include <algorithm>
#include <optional>

using namespace Vulkan::Core;

Buffer::Buffer(const Device& device, size_t size, std::vector<uint8_t> data,
        VkBufferUsageFlags usage) : size(size) {
    if (!device)
        throw std::invalid_argument("Invalid Vulkan device");
    if (size < data.size())
        throw std::invalid_argument("Invalid buffer size");

    // create buffer
    const VkBufferCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VkBuffer bufferHandle{};
    auto res = vkCreateBuffer(device.handle(), &desc, nullptr, &bufferHandle);
    if (res != VK_SUCCESS || bufferHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Failed to create Vulkan buffer");

    // find memory type
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device.getPhysicalDevice(), &memProps);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device.handle(), bufferHandle, &memReqs);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    std::optional<uint32_t> memType{};
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memReqs.memoryTypeBits & (1 << i)) && // NOLINTBEGIN
            (memProps.memoryTypes[i].propertyFlags &
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memType.emplace(i);
            break;
        } // NOLINTEND
    }
    if (!memType.has_value())
        throw ls::vulkan_error(VK_ERROR_UNKNOWN, "Unable to find memory type for buffer");
#pragma clang diagnostic pop

    // allocate and bind memory
    const VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = memType.value()
    };
    VkDeviceMemory memoryHandle{};
    res = vkAllocateMemory(device.handle(), &allocInfo, nullptr, &memoryHandle);
    if (res != VK_SUCCESS || memoryHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Failed to allocate memory for Vulkan buffer");

    res = vkBindBufferMemory(device.handle(), bufferHandle, memoryHandle, 0);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Failed to bind memory to Vulkan buffer");

    // store buffer and memory in shared ptr
    this->buffer = std::shared_ptr<VkBuffer>(
        new VkBuffer(bufferHandle),
        [dev = device.handle()](VkBuffer* img) {
            vkDestroyBuffer(dev, *img, nullptr);
        }
    );
    this->memory = std::shared_ptr<VkDeviceMemory>(
        new VkDeviceMemory(memoryHandle),
        [dev = device.handle()](VkDeviceMemory* mem) {
            vkFreeMemory(dev, *mem, nullptr);
        }
    );

    // upload data to buffer
    uint8_t* buf{};
    res = vkMapMemory(device.handle(), memoryHandle, 0, size, 0, reinterpret_cast<void**>(&buf));
    if (res != VK_SUCCESS || buf == nullptr)
        throw ls::vulkan_error(res, "Failed to map memory for Vulkan buffer");
    std::copy_n(data.data(), size, buf);
    vkUnmapMemory(device.handle(), memoryHandle);
}
