#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "core/sampler.hpp"

#include <array>

namespace Vulkan::Globals {

    /// Global sampler with address mode set to clamp to border.
    extern Core::Sampler samplerClampBorder;
    /// Global sampler with address mode set to clamp to edge.
    extern Core::Sampler samplerClampEdge;

    /// Commonly used constant buffer structure for shaders.
    struct FgBuffer {
        std::array<uint32_t, 2> inputOffset;
        uint32_t firstIter;
        uint32_t firstIterS;
        uint32_t advancedColorKind;
        uint32_t hdrSupport;
        float resolutionInvScale;
        float timestamp;
        float uiThreshold;
        std::array<uint32_t, 3> pad;
    };

    /// Default instance of the FgBuffer.
    extern FgBuffer fgBuffer;

    static_assert(sizeof(FgBuffer) == 48, "FgBuffer must be 48 bytes in size.");

    /// Initialize global resources.
    void initializeGlobals(const Device& device);

    /// Uninitialize global resources.
    void uninitializeGlobals() noexcept;

}

#endif // GLOBAL_HPP
