#include "core/sampler.hpp"
#include "lsfg.hpp"

using namespace LSFG::Core;

Sampler::Sampler(const Device& device, VkSamplerAddressMode mode) {
    // create sampler
    const VkSamplerCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = mode,
        .addressModeV = mode,
        .addressModeW = mode,
        .maxLod = 15.99609F
    };
    VkSampler samplerHandle{};
    auto res = vkCreateSampler(device.handle(), &desc, nullptr, &samplerHandle);
    if (res != VK_SUCCESS || samplerHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to create sampler");

    // store sampler in shared ptr
    this->sampler = std::shared_ptr<VkSampler>(
        new VkSampler(samplerHandle),
        [dev = device.handle()](VkSampler* samplerHandle) {
            vkDestroySampler(dev, *samplerHandle, nullptr);
        }
    );
}
