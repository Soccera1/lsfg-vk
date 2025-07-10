#include "context.hpp"
#include "lsfg.hpp"
#include "utils/utils.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>
#include <cstddef>
#include <algorithm>
#include <optional>
#include <cstdint>

using namespace LSFG;

Context::Context(Vulkan& vk,
        int in0, int in1, const std::vector<int>& outN,
        VkExtent2D extent, VkFormat format) {
    // import input images
    this->inImg_0 = Core::Image(vk.device, extent, format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, in0);
    this->inImg_1 = Core::Image(vk.device, extent, format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, in1);

    // prepare render data
    for (size_t i = 0; i < 8; i++) {
        auto& data = this->data.at(i);
        data.internalSemaphores.resize(outN.size());
        data.outSemaphores.resize(outN.size());
        data.completionFences.resize(outN.size());
        data.cmdBuffers2.resize(outN.size());
    }

    // create shader chains
    this->mipmaps = Shaders::Mipmaps(vk, this->inImg_0, this->inImg_1);
    for (size_t i = 0; i < 7; i++)
        this->alpha.at(i) = Shaders::Alpha(vk, this->mipmaps.getOutImages().at(i));
    this->beta = Shaders::Beta(vk, this->alpha.at(0).getOutImages());
    for (size_t i = 0; i < 7; i++) {
        this->gamma.at(i) = Shaders::Gamma(vk,
            this->alpha.at(6 - i).getOutImages(),
            this->beta.getOutImages().at(std::min<size_t>(6 - i, 5)),
            (i == 0) ? std::nullopt : std::make_optional(this->gamma.at(i - 1).getOutImage()));
        if (i < 4) continue;

        this->delta.at(i - 4) = Shaders::Delta(vk,
            this->alpha.at(6 - i).getOutImages(),
            this->beta.getOutImages().at(6 - i),
            (i == 4) ? std::nullopt : std::make_optional(this->gamma.at(i - 1).getOutImage()),
            (i == 4) ? std::nullopt : std::make_optional(this->delta.at(i - 5).getOutImage1()),
            (i == 4) ? std::nullopt : std::make_optional(this->delta.at(i - 5).getOutImage2()));
    }
    this->generate = Shaders::Generate(vk,
        this->inImg_0, this->inImg_1,
        this->gamma.at(6).getOutImage(),
        this->delta.at(2).getOutImage1(),
        this->delta.at(2).getOutImage2(),
        outN, format);
}

void Context::present(Vulkan& vk,
        int inSem, const std::vector<int>& outSem) {
    auto& data = this->data.at(this->frameIdx % 8);

    // 3. wait for completion of previous frame in this slot
    if (data.shouldWait)
        for (auto& fence : data.completionFences)
            if (!fence.wait(vk.device, UINT64_MAX))
                throw LSFG::vulkan_error(VK_TIMEOUT, "Fence wait timed out");

    // 1. create mipmaps and process input image
    data.inSemaphore = Core::Semaphore(vk.device, inSem);
    for (size_t i = 0; i < outSem.size(); i++)
        data.internalSemaphores.at(i) = Core::Semaphore(vk.device, outSem.at(i));

    data.cmdBuffer1 = Core::CommandBuffer(vk.device, vk.commandPool);
    data.cmdBuffer1.begin();

    this->mipmaps.Dispatch(data.cmdBuffer1, this->frameIdx);
    for (size_t i = 0; i < 7; i++)
        this->alpha.at(6 - i).Dispatch(data.cmdBuffer1, this->frameIdx);
    this->beta.Dispatch(data.cmdBuffer1, this->frameIdx);

    data.cmdBuffer1.end();
    data.cmdBuffer1.submit(vk.device.getComputeQueue(), std::nullopt,
        { data.inSemaphore }, std::nullopt,
        data.internalSemaphores, std::nullopt);

    // 2. generate intermediary frames
    for (size_t i = 0; i < 7; i++) {
        data.outSemaphores.at(i) = Core::Semaphore(vk.device, outSem.at(i));
        data.completionFences.at(i) = Core::Fence(vk.device);

        data.cmdBuffers2.at(i) = Core::CommandBuffer(vk.device, vk.commandPool);
        data.cmdBuffers2.at(i).begin();

        this->gamma.at(i).Dispatch(data.cmdBuffers2.at(i), this->frameIdx, i);
        if (i >= 4)
            this->delta.at(i - 4).Dispatch(data.cmdBuffers2.at(i), this->frameIdx, i);
        this->generate.Dispatch(data.cmdBuffers2.at(i), this->frameIdx, i);

        data.cmdBuffers2.at(i).end();
        data.cmdBuffers2.at(i).submit(vk.device.getComputeQueue(), std::nullopt,
            { data.internalSemaphores.at(i) }, std::nullopt,
            data.outSemaphores, std::nullopt);
    }

    this->frameIdx++;
}
