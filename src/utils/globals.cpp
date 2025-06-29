#include "utils/global.hpp"

using namespace Vulkan;

Core::Sampler Globals::samplerClampBorder;
Core::Sampler Globals::samplerClampEdge;
Globals::FgBuffer Globals::fgBuffer;

void Globals::initializeGlobals(const Device& device) {
    // initialize global samplers
    samplerClampBorder = Core::Sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    samplerClampEdge =   Core::Sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    // initialize global constant buffer
    fgBuffer = {
        .inputOffset = { 0, 29 },
        .resolutionInvScale = 1.0F,
        .timestamp = 0.5F,
        .uiThreshold = 0.1F,
    };

}

void Globals::uninitializeGlobals() noexcept {
    // uninitialize global samplers
    samplerClampBorder = Core::Sampler();
    samplerClampEdge = Core::Sampler();

    // uninitialize global constant buffer
    fgBuffer = Globals::FgBuffer();
}
