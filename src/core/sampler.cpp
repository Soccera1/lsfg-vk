#include "core/sampler.hpp"
#include "utils/exceptions.hpp"

using namespace Vulkan::Core;

Sampler::Sampler(const Device& device, VkSamplerAddressMode mode) {
    if (!device)
        throw std::invalid_argument("Invalid Vulkan device");

    // create sampler
    const VkSamplerCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = mode,
        .addressModeV = mode,
        .addressModeW = mode,
        .maxLod = VK_LOD_CLAMP_NONE
    };
    VkSampler samplerHandle{};
    auto res = vkCreateSampler(device.handle(), &desc, nullptr, &samplerHandle);
    if (res != VK_SUCCESS || samplerHandle == VK_NULL_HANDLE)
        throw ls::vulkan_error(res, "Unable to create sampler");

    // store the sampler in a shared pointer
    this->sampler = std::shared_ptr<VkSampler>(
        new VkSampler(samplerHandle),
        [dev = device.handle()](VkSampler* samplerHandle) {
            vkDestroySampler(dev, *samplerHandle, nullptr);
        }
    );
}
