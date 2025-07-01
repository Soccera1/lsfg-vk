#include "lsfg.hpp"
#include "core/commandbuffer.hpp"
#include "core/fence.hpp"
#include "core/image.hpp"

#include <cassert>
#include <format>

using namespace LSFG;

Generator::Generator(const Context& context) {
    // TEST: create temporal images
    this->inImg_0 = Core::Image(context.device,
        { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    this->inImg_1 = Core::Image(context.device,
        { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    // TEST END

    // create shader chains
    this->downsampleChain = Shaderchains::Downsample(context.device, context.descPool,
        this->inImg_0, this->inImg_1);
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(i) = Shaderchains::Alpha(context.device, context.descPool,
            this->downsampleChain.getOutImages().at(i));
    this->betaChain = Shaderchains::Beta(context.device, context.descPool,
        this->alphaChains.at(0).getOutImages0(),
        this->alphaChains.at(0).getOutImages2(),
        this->alphaChains.at(0).getOutImages1());
    for (size_t i = 0; i < 7; i++) {
        if (i < 4) {
            this->gammaChains.at(i) = Shaderchains::Gamma(context.device, context.descPool,
                this->alphaChains.at(6 - i).getOutImages0(),
                this->alphaChains.at(6 - i).getOutImages2(),
                this->alphaChains.at(6 - i).getOutImages1(),
                this->betaChain.getOutImages().at(std::min(5UL, 6 - i)),
                i == 0 ? std::nullopt
                       : std::optional{this->gammaChains.at(i - 1).getOutImage2()},
                i == 0 ? std::nullopt
                       : std::optional{this->gammaChains.at(i - 1).getOutImage1()},
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent()
            );
        } else {
            this->magicChains.at(i - 4) = Shaderchains::Magic(context.device, context.descPool,
                this->alphaChains.at(6 - i).getOutImages0(),
                this->alphaChains.at(6 - i).getOutImages2(),
                this->alphaChains.at(6 - i).getOutImages1(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage2()
                       : this->extractChains.at(i - 5).getOutImage(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage1()
                       : this->zetaChains.at(i - 5).getOutImage(),
                i == 4 ? std::nullopt : std::optional{this->epsilonChains.at(i - 5).getOutImage()}
            );
            this->deltaChains.at(i - 4) = Shaderchains::Delta(context.device, context.descPool,
                this->magicChains.at(i - 4).getOutImages1(),
                i == 4 ? std::nullopt
                       : std::optional{this->deltaChains.at(i - 5).getOutImage()}
            );
            this->epsilonChains.at(i - 4) = Shaderchains::Epsilon(context.device, context.descPool,
                this->magicChains.at(i - 4).getOutImages2(),
                this->betaChain.getOutImages().at(6 - i),
                i == 4 ? std::nullopt
                       : std::optional{this->epsilonChains.at(i - 5).getOutImage()}
            );
            this->zetaChains.at(i - 4) = Shaderchains::Zeta(context.device, context.descPool,
                this->magicChains.at(i - 4).getOutImages3(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage1()
                       : this->zetaChains.at(i - 5).getOutImage(),
                this->betaChain.getOutImages().at(6 - i)
            );
            if (i >= 6)
                continue; // no extract for i >= 6
            this->extractChains.at(i - 4) = Shaderchains::Extract(context.device, context.descPool,
                this->zetaChains.at(i - 4).getOutImage(),
                this->epsilonChains.at(i - 4).getOutImage(),
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent()
            );
        }
    }
    this->mergeChain = Shaderchains::Merge(context.device, context.descPool,
        this->inImg_1,
        this->inImg_0,
        this->zetaChains.at(2).getOutImage(),
        this->epsilonChains.at(2).getOutImage(),
        this->deltaChains.at(2).getOutImage()
    );
}

void Generator::present(const Context& context) {
    // TEST: upload input images
    // TODO: implement test, lol
    // TEST END

    Core::CommandBuffer cmdBuffer(context.device, context.cmdPool);
    cmdBuffer.begin();

    this->downsampleChain.Dispatch(cmdBuffer, fc);
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(6 - i).Dispatch(cmdBuffer, fc);
    this->betaChain.Dispatch(cmdBuffer, fc);
    for (size_t i = 0; i < 4; i++)
        this->gammaChains.at(i).Dispatch(cmdBuffer, fc);
    for (size_t i = 0; i < 3; i++) {
        this->magicChains.at(i).Dispatch(cmdBuffer, fc);
        this->deltaChains.at(i).Dispatch(cmdBuffer);
        this->epsilonChains.at(i).Dispatch(cmdBuffer);
        this->zetaChains.at(i).Dispatch(cmdBuffer);
        if (i < 2)
            this->extractChains.at(i).Dispatch(cmdBuffer);
    }
    this->mergeChain.Dispatch(cmdBuffer, fc);

    cmdBuffer.end();

    // TEST: temporary fence submit logic
    Core::Fence fence(context.device);
    cmdBuffer.submit(context.device.getComputeQueue(), fence);
    assert(fence.wait(context.device));
    // TEST END

    fc++;
}

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<int32_t>(result))), result(result) {}

vulkan_error::~vulkan_error() noexcept = default;
