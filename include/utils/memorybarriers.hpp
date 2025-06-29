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
    /// @param srcLayout Optional source layout for the images, defaults to VK_IMAGE_LAYOUT_GENERAL
    ///
    /// @throws std::logic_error if the command buffer is not in Recording state
    ///
    void insertBarrier(
        const Vulkan::Core::CommandBuffer& buffer,
        const std::vector<Vulkan::Core::Image>& r2wImages,
        const std::vector<Vulkan::Core::Image>& w2rImages,
        std::optional<VkImageLayout> srcLayout = std::nullopt);

    ///
    /// Insert a global memory barrier in a command buffer.
    ///
    /// @param buffer Command buffer to insert the barrier into
    ///
    /// @throws std::logic_error if the command buffer is not in Recording state
    ///
    void insertGlobalBarrier(const Vulkan::Core::CommandBuffer& buffer);

};

#endif // BARRIERS_HPP
