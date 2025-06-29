#ifndef DEVICE_HPP
#define DEVICE_HPP

#include "instance.hpp"
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

namespace Vulkan {

    ///
    /// C++ wrapper class for a Vulkan device.
    ///
    /// This class manages the lifetime of a Vulkan device.
    ///
    class Device {
    public:
        ///
        /// Create the device.
        ///
        /// @param instance Vulkan instance
        ///
        /// @throws std::invalid_argument if the instance is invalid.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Device(const Vulkan::Instance& instance);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->device; }
        /// Get the physical device associated with this logical device.
        [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return this->physicalDevice; }
        /// Get the compute queue family index.
        [[nodiscard]] uint32_t getComputeFamilyIdx() const { return this->computeFamilyIdx; }
        /// Get the compute queue.
        [[nodiscard]] VkQueue getComputeQueue() const { return this->computeQueue; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->device); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        // Trivially copyable, moveable and destructible
        Device(const Device&) noexcept = default;
        Device& operator=(const Device&) noexcept = default;
        Device(Device&&) noexcept = default;
        Device& operator=(Device&&) noexcept = default;
        ~Device() = default;
    private:
        std::shared_ptr<VkDevice> device;
        VkPhysicalDevice physicalDevice{};

        uint32_t computeFamilyIdx{0};

        VkQueue computeQueue{};
    };

}

#endif // DEVICE_HPP
