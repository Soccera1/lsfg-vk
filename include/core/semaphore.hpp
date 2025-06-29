#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <optional>
#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan semaphore.
    ///
    /// This class manages the lifetime of a Vulkan semaphore.
    ///
    class Semaphore {
    public:
        ///
        /// Create the semaphore.
        ///
        /// @param device Vulkan device
        /// @param initial Optional initial value for creating a timeline semaphore.
        ///
        /// @throws std::invalid_argument if the device is null.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Semaphore(const Device& device, std::optional<uint32_t> initial = std::nullopt);

        ///
        /// Signal the semaphore to a specific value.
        ///
        /// @param value The value to signal the semaphore to.
        ///
        /// @throws std::logic_error if the semaphore is not a timeline semaphore.
        /// @throws ls::vulkan_error if signaling fails.
        ///
        void signal(uint64_t value) const;

        ///
        /// Wait for the semaphore to reach a specific value.
        ///
        /// @param value The value to wait for.
        /// @param timeout The timeout in nanoseconds, or UINT64_MAX for no timeout.
        /// @returns true if the semaphore reached the value, false if it timed out.
        ///
        /// @throws std::logic_error if the semaphore is not a timeline semaphore.
        /// @throws ls::vulkan_error if waiting fails.
        ///
        [[nodiscard]] bool wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->semaphore; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->semaphore); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        // Trivially copyable, moveable and destructible
        Semaphore(const Semaphore&) noexcept = default;
        Semaphore& operator=(const Semaphore&) noexcept = default;
        Semaphore(Semaphore&&) noexcept = default;
        Semaphore& operator=(Semaphore&&) noexcept = default;
        ~Semaphore() = default;
    private:
        std::shared_ptr<VkSemaphore> semaphore;
        VkDevice device{};
        bool isTimeline{};
    };

}

#endif // SEMAPHORE_HPP
