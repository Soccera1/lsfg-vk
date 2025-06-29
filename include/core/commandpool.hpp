#ifndef COMMANDPOOL_HPP
#define COMMANDPOOL_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    /// Enumeration for different types of command pools.
    enum class CommandPoolType {
        /// Used for compute-type command buffers.
        Compute
    };

    ///
    /// C++ wrapper class for a Vulkan command pool.
    ///
    /// This class manages the lifetime of a Vulkan command pool.
    ///
    class CommandPool {
    public:
        ///
        /// Create the command pool.
        ///
        /// @param device Vulkan device
        /// @param type Type of command pool to create.
        ///
        /// @throws std::invalid_argument if the device is invalid.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        CommandPool(const Device& device, CommandPoolType type);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->commandPool; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->commandPool); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        /// Trivially copyable, moveable and destructible
        CommandPool(const CommandPool&) noexcept = default;
        CommandPool& operator=(const CommandPool&) noexcept = default;
        CommandPool(CommandPool&&) noexcept = default;
        CommandPool& operator=(CommandPool&&) noexcept = default;
        ~CommandPool() = default;
    private:
        std::shared_ptr<VkCommandPool> commandPool;
    };

}

#endif // COMMANDPOOL_HPP
