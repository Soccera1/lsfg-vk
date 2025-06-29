#ifndef COMMANDPOOL_HPP
#define COMMANDPOOL_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

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
        ///
        /// @throws ls::vulkan_error if object creation fails.
        ///
        CommandPool(const Device& device);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->commandPool; }

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
