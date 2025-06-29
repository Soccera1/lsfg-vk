#ifndef DESCRIPTORPOOL_HPP
#define DESCRIPTORPOOL_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan descriptor pool.
    ///
    /// This class manages the lifetime of a Vulkan descriptor pool.
    ///
    class DescriptorPool {
    public:
        ///
        /// Create the descriptor pool.
        ///
        /// @param device Vulkan device
        ///
        /// @throws std::invalid_argument if the device is invalid.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        DescriptorPool(const Device& device);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->descriptorPool; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->descriptorPool); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        /// Trivially copyable, moveable and destructible
        DescriptorPool(const DescriptorPool&) noexcept = default;
        DescriptorPool& operator=(const DescriptorPool&) noexcept = default;
        DescriptorPool(DescriptorPool&&) noexcept = default;
        DescriptorPool& operator=(DescriptorPool&&) noexcept = default;
        ~DescriptorPool() = default;
    private:
        std::shared_ptr<VkDescriptorPool> descriptorPool;
    };

}

#endif // DESCRIPTORPOOL_HPP
