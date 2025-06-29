#ifndef FENCE_HPP
#define FENCE_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan fence.
    ///
    /// This class manages the lifetime of a Vulkan fence.
    ///
    class Fence {
    public:
        ///
        /// Create the fence.
        ///
        /// @param device Vulkan device
        ///
        /// @throws std::invalid_argument if the device is null.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Fence(const Device& device);

        ///
        /// Reset the fence to an unsignaled state.
        ///
        /// @throws std::logic_error if the fence is not valid.
        /// @throws ls::vulkan_error if resetting fails.
        ///
        void reset() const;

        ///
        /// Wait for the fence
        ///
        /// @param timeout The timeout in nanoseconds, or UINT64_MAX for no timeout.
        /// @returns true if the fence signaled, false if it timed out.
        ///
        /// @throws std::logic_error if the fence is not valid.
        /// @throws ls::vulkan_error if waiting fails.
        ///
        [[nodiscard]] bool wait(uint64_t timeout = UINT64_MAX) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->fence; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->fence); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        // Trivially copyable, moveable and destructible
        Fence(const Fence&) noexcept = default;
        Fence& operator=(const Fence&) noexcept = default;
        Fence(Fence&&) noexcept = default;
        Fence& operator=(Fence&&) noexcept = default;
        ~Fence() = default;
    private:
        std::shared_ptr<VkFence> fence;
        VkDevice device{};
    };

}

#endif // FENCE_HPP
