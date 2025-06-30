#include "core/commandbuffer.hpp"
#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/fence.hpp"
#include "core/image.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "shaderchains/downsample.hpp"
#include "utils.hpp"

#include <iostream>

#include <renderdoc_app.h>
#include <dlfcn.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

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

    // create initialization resources
    Core::Image inputImage(
        device, { 2560, 1411 }, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
    Utils::uploadImage(device, commandPool, inputImage, "rsc/images/source.dds");

    // create the shaderchains
    Shaderchains::Downsample downsample(device, descriptorPool, inputImage);

    // start the rendering pipeline
    if (rdoc)
        rdoc->StartFrameCapture(nullptr, nullptr);

    Core::CommandBuffer commandBuffer(device, commandPool);
    commandBuffer.begin();

    downsample.Dispatch(commandBuffer);

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
