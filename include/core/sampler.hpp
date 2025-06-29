#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan sampler.
    ///
    /// This class manages the lifetime of a Vulkan sampler.
    ///
    class Sampler {
    public:
        ///
        /// Create the sampler.
        ///
        /// @param device Vulkan device
        /// @param mode Address mode for the sampler.
        ///
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Sampler(const Device& device, VkSamplerAddressMode mode);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->sampler; }

        /// Trivially copyable, moveable and destructible
        Sampler(const Sampler&) noexcept = default;
        Sampler& operator=(const Sampler&) noexcept = default;
        Sampler(Sampler&&) noexcept = default;
        Sampler& operator=(Sampler&&) noexcept = default;
        ~Sampler() = default;
    private:
        std::shared_ptr<VkSampler> sampler;
    };

}

#endif // SAMPLER_HPP
