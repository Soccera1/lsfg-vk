#include "utils/upload.hpp"
#include "utils/exceptions.hpp"

#include <fstream>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace Upload;

void Upload::upload(const Vulkan::Device& device,
        Vulkan::Core::Image& image, const std::string& path) {
    auto vkTransitionImageLayoutEXT = reinterpret_cast<PFN_vkTransitionImageLayoutEXT>(
        vkGetDeviceProcAddr(device.handle(), "vkTransitionImageLayoutEXT"));
    auto vkCopyMemoryToImageEXT = reinterpret_cast<PFN_vkCopyMemoryToImageEXT>(
        vkGetDeviceProcAddr(device.handle(), "vkCopyMemoryToImageEXT"));

    const VkHostImageLayoutTransitionInfoEXT transitionInfo{
        .sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
        .image = image.handle(),
        .oldLayout = image.getLayout(),
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .subresourceRange = {
            .aspectMask = image.getAspectFlags(),
            .levelCount = 1,
            .layerCount = 1
        }
    };
    image.setLayout(VK_IMAGE_LAYOUT_GENERAL);
    auto res = vkTransitionImageLayoutEXT(device.handle(), 1, &transitionInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Failed to transition image layout for upload");

    // read shader bytecode
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file)
        throw std::system_error(errno, std::generic_category(), "Failed to open shader file: " + path);

    std::streamsize size = file.tellg();
    size -= 124 - 4;
    std::vector<uint8_t> code(static_cast<size_t>(size));

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(code.data()), size))
        throw std::system_error(errno, std::generic_category(), "Failed to read shader file: " + path);

    file.close();

    // copy data to image
    auto extent = image.getExtent();
    const VkMemoryToImageCopyEXT copyInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT,
        .pHostPointer = code.data(),
        .imageSubresource = {
            .aspectMask = image.getAspectFlags(),
            .layerCount = 1
        },
        .imageExtent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        },
    };

    const VkCopyMemoryToImageInfoEXT operationInfo{
        .sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT,
        .dstImage = image.handle(),
        .dstImageLayout = image.getLayout(),
        .regionCount = 1,
        .pRegions = &copyInfo,
    };
    res = vkCopyMemoryToImageEXT(device.handle(), &operationInfo);
    if (res != VK_SUCCESS)
        throw ls::vulkan_error(res, "Failed to copy memory to image");
}
