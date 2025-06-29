#ifndef UPLOAD_HPP
#define UPLOAD_HPP

#include "core/commandpool.hpp"
#include "core/image.hpp"
#include "device.hpp"

#include <string>

namespace Upload {

    ///
    /// Upload a DDS file to a Vulkan image.
    ///
    /// @param device The Vulkan device
    /// @param commandPool The command pool
    /// @param image The Vulkan image to upload to
    /// @param path The path to the DDS file.
    ///
    /// @throws std::system_error If the file cannot be opened or read.
    /// @throws ls:vulkan_error If the Vulkan image cannot be created or updated.
    ///
    void upload(const Vulkan::Device& device, const Vulkan::Core::CommandPool& commandPool,
        Vulkan::Core::Image& image, const std::string& path);

}

#endif // UPLOAD_HPP
