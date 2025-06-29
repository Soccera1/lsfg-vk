#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>
#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan buffer.
    ///
    /// This class manages the lifetime of a Vulkan buffer.
    ///
    class Buffer {
    public:
        ///
        /// Create the buffer.
        ///
        /// @param device Vulkan device
        /// @param size Size of the buffer in bytes.
        /// @param data Initial data for the buffer.
        /// @param usage Usage flags for the buffer
        ///
        /// @throws std::invalid_argument if the device or buffer size is invalid
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Buffer(const Device& device, uint32_t size, std::vector<uint8_t> data,
            VkBufferUsageFlags usage);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->buffer; }
        /// Get the size of the buffer.
        [[nodiscard]] uint32_t getSize() const { return this->size; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->buffer); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        /// Trivially copyable, moveable and destructible
        Buffer(const Buffer&) noexcept = default;
        Buffer& operator=(const Buffer&) noexcept = default;
        Buffer(Buffer&&) noexcept = default;
        Buffer& operator=(Buffer&&) noexcept = default;
        ~Buffer() = default;
    private:
        std::shared_ptr<VkBuffer> buffer;
        std::shared_ptr<VkDeviceMemory> memory;

        uint32_t size;
    };

}

#endif // BUFFER_HPP
