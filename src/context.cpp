#include "context.hpp"
#include "lsfg.hpp"

#include <cassert>
#include <format>

using namespace LSFG;

Context::Context(const Core::Device& device, uint32_t width, uint32_t height, int in0, int in1) {
    // create pools
    this->descPool = Core::DescriptorPool(device);
    this->cmdPool = Core::CommandPool(device);

    // create shader chains
    this->downsampleChain = Shaderchains::Downsample(device, this->descPool,
        this->inImg_0, this->inImg_1);
    for (size_t i = 0; i < 7; i++)
        this->alphaChains.at(i) = Shaderchains::Alpha(device, this->descPool,
            this->downsampleChain.getOutImages().at(i));
    this->betaChain = Shaderchains::Beta(device, this->descPool,
        this->alphaChains.at(0).getOutImages0(),
        this->alphaChains.at(0).getOutImages1(),
        this->alphaChains.at(0).getOutImages2());
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
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent()
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
                i == 4 ? std::nullopt : std::optional{this->epsilonChains.at(i - 5).getOutImage()}
            );
            this->deltaChains.at(i - 4) = Shaderchains::Delta(device, this->descPool,
                this->magicChains.at(i - 4).getOutImages1(),
                i == 4 ? std::nullopt
                       : std::optional{this->deltaChains.at(i - 5).getOutImage()}
            );
            this->epsilonChains.at(i - 4) = Shaderchains::Epsilon(device, this->descPool,
                this->magicChains.at(i - 4).getOutImages2(),
                this->betaChain.getOutImages().at(6 - i),
                i == 4 ? std::nullopt
                       : std::optional{this->epsilonChains.at(i - 5).getOutImage()}
            );
            this->zetaChains.at(i - 4) = Shaderchains::Zeta(device, this->descPool,
                this->magicChains.at(i - 4).getOutImages3(),
                i == 4 ? this->gammaChains.at(i - 1).getOutImage1()
                       : this->zetaChains.at(i - 5).getOutImage(),
                this->betaChain.getOutImages().at(6 - i)
            );
            if (i >= 6)
                continue; // no extract for i >= 6
            this->extractChains.at(i - 4) = Shaderchains::Extract(device, this->descPool,
                this->zetaChains.at(i - 4).getOutImage(),
                this->epsilonChains.at(i - 4).getOutImage(),
                this->alphaChains.at(6 - i - 1).getOutImages0().at(0).getExtent()
            );
        }
    }
    this->mergeChain = Shaderchains::Merge(device, this->descPool,
        this->inImg_1,
        this->inImg_0,
        this->zetaChains.at(2).getOutImage(),
        this->epsilonChains.at(2).getOutImage(),
        this->deltaChains.at(2).getOutImage()
    );
}

void Context::present(const Core::Device& device, int inSem, int outSem) {
    Core::CommandBuffer cmdBuffer(device, this->cmdPool);
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

    fc++;
}

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<int32_t>(result))), result(result) {}

vulkan_error::~vulkan_error() noexcept = default;
