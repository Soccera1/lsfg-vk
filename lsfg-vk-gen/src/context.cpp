#include "context.hpp"
#include "core/fence.hpp"
#include "core/semaphore.hpp"
#include "lsfg.hpp"

#include <cassert>
#include <format>
#include <optional>
#include <vulkan/vulkan_core.h>

using namespace LSFG;

Context::Context(const Core::Device& device, uint32_t width, uint32_t height, int in0, int in1,
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

    // create pools
    this->descPool = Core::DescriptorPool(device);
    this->cmdPool = Core::CommandPool(device);

    // prepare vectors
    for (size_t i = 0; i < 8; i++) {
        this->internalSemaphores.at(i).resize(outN.size());
        this->doneFences.at(i).resize(outN.size());
    }
    for (size_t i = 0; i < outN.size(); i++) {
        this->outSemaphores.emplace_back();
        this->cmdBuffers2.emplace_back();
    }

    // create shader chains
    this->downsampleChain = Shaderchains::Downsample(device, this->descPool,
        this->inImg_0, this->inImg_1, outN.size());
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(i) = Shaderchains::Alpha(device, this->descPool,
            this->downsampleChain.getOutImages().at(i));
    this->betaChain = Shaderchains::Beta(device, this->descPool,
        this->alphaChains.at(0).getOutImages0(),
        this->alphaChains.at(0).getOutImages1(),
        this->alphaChains.at(0).getOutImages2(), outN.size());
    for (size_t i = 0; i < 7; i++) {
        if (i < 4) {
            this->gammaChains.at(i) = Shaderchains::Gamma(device, this->descPool,
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
            this->magicChains.at(i - 4) = Shaderchains::Magic(device, this->descPool,
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
            this->deltaChains.at(i - 4) = Shaderchains::Delta(device, this->descPool,
                this->magicChains.at(i - 4).getOutImages1(),
                i == 4 ? std::nullopt
                       : std::optional{this->deltaChains.at(i - 5).getOutImage()},
                       outN.size()
            );
            this->epsilonChains.at(i - 4) = Shaderchains::Epsilon(device, this->descPool,
                this->magicChains.at(i - 4).getOutImages2(),
                this->betaChain.getOutImages().at(6 - i),
                i == 4 ? std::nullopt
                       : std::optional{this->epsilonChains.at(i - 5).getOutImage()},
                       outN.size()
            );
            this->zetaChains.at(i - 4) = Shaderchains::Zeta(device, this->descPool,
                this->magicChains.at(i - 4).getOutImages3(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage1()
                       : this->zetaChains.at(i - 5).getOutImage(),
                this->betaChain.getOutImages().at(6 - i),
                outN.size()
            );
            if (i >= 6)
                continue; // no extract for i >= 6
            this->extractChains.at(i - 4) = Shaderchains::Extract(device, this->descPool,
                this->zetaChains.at(i - 4).getOutImage(),
                this->epsilonChains.at(i - 4).getOutImage(),
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent(),
                outN.size()
            );
        }
    }
    this->mergeChain = Shaderchains::Merge(device, this->descPool,
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
    auto& doneFences = this->doneFences.at(this->fc % 8);
    for (auto& fenceOptional : doneFences) {
        if (!fenceOptional.has_value())
            continue;
        if (!fenceOptional->wait(device, UINT64_MAX))
            throw vulkan_error(VK_ERROR_DEVICE_LOST, "Fence wait timed out");
    }

    auto& inSemaphore = this->inSemaphores.at(this->fc % 8);
    inSemaphore = Core::Semaphore(device, inSem);
    auto& internalSemaphores = this->internalSemaphores.at(this->fc % 8);
    for (size_t i = 0; i < outSem.size(); i++)
        internalSemaphores.at(i) = Core::Semaphore(device);

    auto& cmdBuffer1 = this->cmdBuffers1.at(this->fc % 8);
    cmdBuffer1 = Core::CommandBuffer(device, this->cmdPool);
    cmdBuffer1.begin();

    this->downsampleChain.Dispatch(cmdBuffer1, fc);
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(6 - i).Dispatch(cmdBuffer1, fc);

    cmdBuffer1.end();

    cmdBuffer1.submit(device.getComputeQueue(), std::nullopt,
        { inSemaphore }, std::nullopt,
        internalSemaphores, std::nullopt);

    for (size_t pass = 0; pass < outSem.size(); pass++) {
        auto& outSemaphore = this->outSemaphores.at(pass).at(this->fc % 8);
        outSemaphore = Core::Semaphore(device, outSem.at(pass));
        auto& outFenceOptional = this->doneFences.at(fc % 8).at(pass);
        outFenceOptional.emplace(Core::Fence(device));

        auto& cmdBuffer2 = this->cmdBuffers2.at(pass).at(this->fc % 8);
        cmdBuffer2 = Core::CommandBuffer(device, this->cmdPool);
        cmdBuffer2.begin();

        this->betaChain.Dispatch(cmdBuffer2, fc, pass);
        for (size_t i = 0; i < 4; i++)
            this->gammaChains.at(i).Dispatch(cmdBuffer2, fc, pass);
        for (size_t i = 0; i < 3; i++) {
            this->magicChains.at(i).Dispatch(cmdBuffer2, fc, pass);
            this->deltaChains.at(i).Dispatch(cmdBuffer2, pass);
            this->epsilonChains.at(i).Dispatch(cmdBuffer2, pass);
            this->zetaChains.at(i).Dispatch(cmdBuffer2, pass);
            if (i < 2)
                this->extractChains.at(i).Dispatch(cmdBuffer2, pass);
        }
        this->mergeChain.Dispatch(cmdBuffer2, fc, pass);

        cmdBuffer2.end();

        cmdBuffer2.submit(device.getComputeQueue(), outFenceOptional,
            { internalSemaphores.at(pass) }, std::nullopt,
            { outSemaphore }, std::nullopt);
    }

    fc++;
}

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<int32_t>(result))), result(result) {}

vulkan_error::~vulkan_error() noexcept = default;
