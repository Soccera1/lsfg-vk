#include "context.hpp"
#include "core/fence.hpp"
#include "core/semaphore.hpp"
#include "pool/shaderpool.hpp"
#include "lsfg.hpp"

#include <vulkan/vulkan_core.h>

#include <format>
#include <optional>

using namespace LSFG;

Context::Context(const Core::Device& device, Pool::ShaderPool& shaderpool,
        uint32_t width, uint32_t height, int in0, int in1,
        const std::vector<int>& outN) {
    // import images
    this->inImg_0 = Core::Image(device, { width, height },
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        in0);
    this->inImg_1 = Core::Image(device, { width, height },
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        in1);

    // prepare render infos
    this->descPool = Core::DescriptorPool(device);
    this->cmdPool = Core::CommandPool(device);
    for (size_t i = 0; i < 8; i++) {
        auto& info = this->renderInfos.at(i);
        info.internalSemaphores.resize(outN.size());
        info.cmdBuffers2.resize(outN.size());
        info.outSemaphores.resize(outN.size());
    }

    // create shader chains
    this->downsampleChain = Shaderchains::Downsample(device, shaderpool, this->descPool,
        this->inImg_0, this->inImg_1, outN.size());
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(i) = Shaderchains::Alpha(device, shaderpool, this->descPool,
            this->downsampleChain.getOutImages().at(i));
    this->betaChain = Shaderchains::Beta(device, shaderpool, this->descPool,
        this->alphaChains.at(0).getOutImages0(),
        this->alphaChains.at(0).getOutImages1(),
        this->alphaChains.at(0).getOutImages2(), outN.size());
    for (size_t i = 0; i < 7; i++) {
        if (i < 4) {
            this->gammaChains.at(i) = Shaderchains::Gamma(device, shaderpool, this->descPool,
                this->alphaChains.at(6 - i).getOutImages0(),
                this->alphaChains.at(6 - i).getOutImages1(),
                this->alphaChains.at(6 - i).getOutImages2(),
                this->betaChain.getOutImages().at(std::min(5UL, 6 - i)),
                i == 0 ? std::nullopt
                       : std::optional{this->gammaChains.at(i - 1).getOutImage2()},
                i == 0 ? std::nullopt
                       : std::optional{this->gammaChains.at(i - 1).getOutImage1()},
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent(),
                outN.size()
            );
        } else {
            this->magicChains.at(i - 4) = Shaderchains::Magic(device, shaderpool, this->descPool,
                this->alphaChains.at(6 - i).getOutImages0(),
                this->alphaChains.at(6 - i).getOutImages1(),
                this->alphaChains.at(6 - i).getOutImages2(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage2()
                       : this->extractChains.at(i - 5).getOutImage(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage1()
                       : this->zetaChains.at(i - 5).getOutImage(),
                i == 4 ? std::nullopt : std::optional{this->epsilonChains.at(i - 5).getOutImage()},
                outN.size()
            );
            this->deltaChains.at(i - 4) = Shaderchains::Delta(device, shaderpool, this->descPool,
                this->magicChains.at(i - 4).getOutImages1(),
                i == 4 ? std::nullopt
                       : std::optional{this->deltaChains.at(i - 5).getOutImage()},
                       outN.size()
            );
            this->epsilonChains.at(i - 4) = Shaderchains::Epsilon(device, shaderpool, this->descPool,
                this->magicChains.at(i - 4).getOutImages2(),
                this->betaChain.getOutImages().at(6 - i),
                i == 4 ? std::nullopt
                       : std::optional{this->epsilonChains.at(i - 5).getOutImage()},
                       outN.size()
            );
            this->zetaChains.at(i - 4) = Shaderchains::Zeta(device, shaderpool, this->descPool,
                this->magicChains.at(i - 4).getOutImages3(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage1()
                       : this->zetaChains.at(i - 5).getOutImage(),
                this->betaChain.getOutImages().at(6 - i),
                outN.size()
            );
            if (i >= 6)
                continue; // no extract for i >= 6
            this->extractChains.at(i - 4) = Shaderchains::Extract(device, shaderpool, this->descPool,
                this->zetaChains.at(i - 4).getOutImage(),
                this->epsilonChains.at(i - 4).getOutImage(),
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent(),
                outN.size()
            );
        }
    }
    this->mergeChain = Shaderchains::Merge(device, shaderpool, this->descPool,
        this->inImg_1,
        this->inImg_0,
        this->zetaChains.at(2).getOutImage(),
        this->epsilonChains.at(2).getOutImage(),
        this->deltaChains.at(2).getOutImage(),
        outN,
        outN.size()
    );
}

void Context::present(const Core::Device& device, int inSem,
        const std::vector<int>& outSem) {
    auto& info = this->renderInfos.at(this->frameIdx % 8);

    // 3. wait for completion of previous frame in this slot
    if (info.completionFences.has_value())
        for (auto& fence : *info.completionFences)
            if (!fence.wait(device, UINT64_MAX)) // should not take any time
                throw vulkan_error(VK_ERROR_DEVICE_LOST, "Fence wait timed out");

    // 1. downsample and process input image
    info.inSemaphore = Core::Semaphore(device, inSem);
    for (size_t i = 0; i < outSem.size(); i++)
        info.internalSemaphores.at(i) = Core::Semaphore(device);

    info.cmdBuffer1 = Core::CommandBuffer(device, this->cmdPool);
    info.cmdBuffer1.begin();

    this->downsampleChain.Dispatch(info.cmdBuffer1, this->frameIdx);
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(6 - i).Dispatch(info.cmdBuffer1, this->frameIdx);

    info.cmdBuffer1.end();
    info.cmdBuffer1.submit(device.getComputeQueue(), std::nullopt,
        { info.inSemaphore }, std::nullopt,
        info.internalSemaphores, std::nullopt);

    // 2. generate intermediary frames
    info.completionFences.emplace();
    for (size_t pass = 0; pass < outSem.size(); pass++) {
        auto& completionFence = info.completionFences->emplace_back(device);
        auto& outSemaphore = info.outSemaphores.at(pass);
        outSemaphore = Core::Semaphore(device, outSem.at(pass));

        auto& cmdBuffer2 = info.cmdBuffers2.at(pass);
        cmdBuffer2 = Core::CommandBuffer(device, this->cmdPool);
        cmdBuffer2.begin();

        this->betaChain.Dispatch(cmdBuffer2, this->frameIdx, pass);
        for (size_t i = 0; i < 4; i++)
            this->gammaChains.at(i).Dispatch(cmdBuffer2, this->frameIdx, pass);
        for (size_t i = 0; i < 3; i++) {
            this->magicChains.at(i).Dispatch(cmdBuffer2, this->frameIdx, pass);
            this->deltaChains.at(i).Dispatch(cmdBuffer2, pass);
            this->epsilonChains.at(i).Dispatch(cmdBuffer2, pass);
            this->zetaChains.at(i).Dispatch(cmdBuffer2, pass);
            if (i < 2)
                this->extractChains.at(i).Dispatch(cmdBuffer2, pass);
        }
        this->mergeChain.Dispatch(cmdBuffer2, this->frameIdx, pass);

        cmdBuffer2.end();

        cmdBuffer2.submit(device.getComputeQueue(), completionFence,
            { info.internalSemaphores.at(pass) }, std::nullopt,
            { outSemaphore }, std::nullopt);
    }

    this->frameIdx++;
}

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<int32_t>(result))), result(result) {}

vulkan_error::~vulkan_error() noexcept = default;
