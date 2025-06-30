#include "core/commandbuffer.hpp"
#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/fence.hpp"
#include "core/image.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "shaderchains/alpha.hpp"
#include "shaderchains/beta.hpp"
#include "shaderchains/delta.hpp"
#include "shaderchains/downsample.hpp"
#include "shaderchains/epsilon.hpp"
#include "shaderchains/extract.hpp"
#include "shaderchains/gamma.hpp"
#include "shaderchains/magic.hpp"
#include "shaderchains/merge.hpp"
#include "shaderchains/zeta.hpp"
#include "utils.hpp"

#include <format>
#include <iostream>

#include <renderdoc_app.h>
#include <dlfcn.h>
#include <unistd.h>
#include <vector>

using namespace Vulkan;

int main() {
    // attempt to load renderdoc
    RENDERDOC_API_1_6_0* rdoc = nullptr;
    if (void* mod_renderdoc = dlopen("/usr/lib/librenderdoc.so", RTLD_NOLOAD | RTLD_NOW)) {
        std::cerr << "Found RenderDoc library, setting up frame capture." << '\n';

        auto GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod_renderdoc, "RENDERDOC_GetAPI"));
        const int ret = GetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&rdoc));
        if (ret == 0) {
            std::cerr << "Unable to initialize RenderDoc API. Is your RenderDoc up to date?" << '\n';
            rdoc = nullptr;
        }
        usleep(1000 * 100); // give renderdoc time to load
    }

    // initialize application
    const Instance instance;
    const Device device(instance);
    const Core::DescriptorPool descriptorPool(device);
    const Core::CommandPool commandPool(device);
    const Core::Fence fence(device);

    Globals::initializeGlobals(device);

    // create downsample shader chain
    Core::Image inputImage(
        device, { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    Utils::uploadImage(device, commandPool, inputImage, "rsc/images/source.dds");

    Shaderchains::Downsample downsample(device, descriptorPool, inputImage);

    // create alpha shader chains
    std::vector<Shaderchains::Alpha> alphas;
    alphas.reserve(7);
    for (size_t i = 0; i < 7; ++i)
        alphas.emplace_back(device, descriptorPool, downsample.getOutImages().at(i));

    // create beta shader chain
    std::array<Core::Image, 8> betaTemporalImages;
    auto betaInExtent = alphas.at(0).getOutImages().at(0).getExtent();
    for (size_t i = 0; i < 8; ++i) {
        betaTemporalImages.at(i) = Core::Image(
            device, betaInExtent, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        Utils::uploadImage(device, commandPool, betaTemporalImages.at(i),
            std::format("rsc/images/temporal_beta/{}.dds", i));
    }

    Shaderchains::Beta beta(device, descriptorPool,
        betaTemporalImages,
        alphas.at(0).getOutImages()
    );

    // create gamma to zeta shader chains
    std::vector<Shaderchains::Gamma> gammas;
    std::vector<Shaderchains::Magic> magics;
    std::vector<Shaderchains::Delta> deltas;
    std::vector<Shaderchains::Epsilon> epsilons;
    std::vector<Shaderchains::Zeta> zetas;
    std::vector<Shaderchains::Extract> extracts;
    std::array<std::array<Core::Image, 4>, 7> otherTemporalImages;
    for (size_t i = 0; i < 7; i++) {
        auto otherInExtent = alphas.at(6 - i).getOutImages().at(0).getExtent();
        for (size_t j = 0; j < 4; j++) {
            otherTemporalImages.at(i).at(j) = Core::Image(
                device, otherInExtent, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
            Utils::uploadImage(device, commandPool, otherTemporalImages.at(i).at(j),
                std::format("rsc/images/temporal_other/{}.{}.dds", i, j));
        }
        if (i < 4) {
            auto gammaOutExtent = alphas.at(6 - i - 1).getOutImages().at(0).getExtent();
            gammas.emplace_back(device, descriptorPool,
                otherTemporalImages.at(i),
                alphas.at(6 - i).getOutImages(),
                beta.getOutImages().at(std::min(static_cast<size_t>(5), 6 - i)), // smallest twice
                i == 0 ? std::nullopt : std::optional{gammas.at(i - 1).getOutImage2()},
                i == 0 ? std::nullopt : std::optional{gammas.at(i - 1).getOutImage1()},
                gammaOutExtent);
        } else {
            magics.emplace_back(device, descriptorPool,
                otherTemporalImages.at(i),
                alphas.at(6 - i).getOutImages(),
                i == 4 ? gammas.at(i - 1).getOutImage2() : extracts.at(i - 5).getOutImage(),
                i == 4 ? gammas.at(i - 1).getOutImage1() : zetas.at(i - 5).getOutImage(),
                i == 4 ? std::nullopt : std::optional{epsilons.at(i - 5).getOutImage()}
            );
            deltas.emplace_back(device, descriptorPool,
                magics.at(i - 4).getOutImages1(),
                i == 4 ? std::nullopt : std::optional{deltas.at(i - 5).getOutImage()}
            );
            epsilons.emplace_back(device, descriptorPool,
                magics.at(i - 4).getOutImages2(),
                beta.getOutImages().at(6 - i),
                i == 4 ? std::nullopt : std::optional{epsilons.at(i - 5).getOutImage()}
            );
            zetas.emplace_back(device, descriptorPool,
                magics.at(i - 4).getOutImages3(),
                i == 4 ? gammas.at(i - 1).getOutImage1() : zetas.at(i - 5).getOutImage(),
                beta.getOutImages().at(6 - i)
            );
            if (i < 6) {
                auto extractOutExtent = alphas.at(6 - i - 1).getOutImages().at(0).getExtent();
                extracts.emplace_back(device, descriptorPool,
                    zetas.at(i - 4).getOutImage(),
                    epsilons.at(i - 4).getOutImage(),
                    extractOutExtent);
            }
        }
    }

    // create merge shader chain
    Core::Image inputImagePrev(
        device, { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    Utils::uploadImage(device, commandPool, inputImagePrev, "rsc/images/source_prev.dds");
    Shaderchains::Merge merge(device, descriptorPool,
        inputImagePrev,
        inputImage,
        zetas.at(2).getOutImage(),
        epsilons.at(2).getOutImage(),
        deltas.at(2).getOutImage()
    );

    // start the rendering pipeline
    if (rdoc)
        rdoc->StartFrameCapture(nullptr, nullptr);

    Core::CommandBuffer commandBuffer(device, commandPool);
    commandBuffer.begin();

    downsample.Dispatch(commandBuffer);
    for (size_t i = 0; i < 7; i++)
        alphas.at(6 - i).Dispatch(commandBuffer);
    beta.Dispatch(commandBuffer);
    for (size_t i = 0; i < 4; i++)
        gammas.at(i).Dispatch(commandBuffer);
    for (size_t i = 0; i < 3; i++) {
        magics.at(i).Dispatch(commandBuffer);
        deltas.at(i).Dispatch(commandBuffer);
        epsilons.at(i).Dispatch(commandBuffer);
        zetas.at(i).Dispatch(commandBuffer);
        if (i < 2)
            extracts.at(i).Dispatch(commandBuffer);
    }
    merge.Dispatch(commandBuffer);

    // finish the rendering pipeline
    commandBuffer.end();

    commandBuffer.submit(device.getComputeQueue(), fence);
    if (!fence.wait(device)) {
        Globals::uninitializeGlobals();

        std::cerr << "Application failed due to timeout" << '\n';
        return 1;
    }

    if (rdoc)
        rdoc->EndFrameCapture(nullptr, nullptr);

    usleep(1000 * 100); // give renderdoc time to capture

    Globals::uninitializeGlobals();

    std::cerr << "Application finished" << '\n';
    return 0;
}
