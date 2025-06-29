#ifndef UPLOAD_HPP
#define UPLOAD_HPP

#include "core/image.hpp"
#include "device.hpp"

#include <string>

namespace Upload {

    void upload(const Vulkan::Device& device,
            Vulkan::Core::Image& image, const std::string& path);

}

#endif // UPLOAD_HPP
