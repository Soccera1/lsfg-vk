#ifndef SHADERMODULE_HPP
#define SHADERMODULE_HPP

#include "device.hpp"

#include <string>
#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan shader module.
    ///
    /// This class manages the lifetime of a Vulkan shader module.
    ///
    class ShaderModule {
    public:
        ///
        /// Create the shader module.
        ///
        /// @param device Vulkan device
        /// @param path Path to the shader file.
        ///
        /// @throws std::invalid_argument if the device is invalid.
        /// @throws std::system_error if the shader file cannot be opened or read.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        ShaderModule(const Device& device, const std::string& path);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->shaderModule; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->shaderModule); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        /// Trivially copyable, moveable and destructible
        ShaderModule(const ShaderModule&) noexcept = default;
        ShaderModule& operator=(const ShaderModule&) noexcept = default;
        ShaderModule(ShaderModule&&) noexcept = default;
        ShaderModule& operator=(ShaderModule&&) noexcept = default;
        ~ShaderModule() = default;
    private:
        std::shared_ptr<VkShaderModule> shaderModule;
    };

}

#endif // SHADERMODULE_HPP
