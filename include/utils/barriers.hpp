#ifndef BARRIERS_HPP
#define BARRIERS_HPP

#include "core/commandbuffer.hpp"
#include "core/image.hpp"

#include <vector>

#include <vulkan/vulkan_core.h>

namespace Barriers {

    ///
    /// Insert memory barriers for images in a command buffer.
    ///
    /// @param buffer Command buffer to insert barriers into
    /// @param r2wImages Images that are being read and will be written to
    /// @param w2rImages Images that are being written to and will be read from
    ///
    /// @throws std::logic_error if the command buffer is not in Recording state
    ///
    void insertBarrier(
        const Vulkan::Core::CommandBuffer& buffer,
        std::vector<Vulkan::Core::Image> r2wImages,
        std::vector<Vulkan::Core::Image> w2rImages);

};

#endif // BARRIERS_HPP
