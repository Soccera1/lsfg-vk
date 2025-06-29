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
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Buffer(const Device& device, size_t size, std::vector<uint8_t> data,
            VkBufferUsageFlags usage);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->buffer; }
        /// Get the size of the buffer.
        [[nodiscard]] size_t getSize() const { return this->size; }

        /// Trivially copyable, moveable and destructible
        Buffer(const Buffer&) noexcept = default;
        Buffer& operator=(const Buffer&) noexcept = default;
        Buffer(Buffer&&) noexcept = default;
        Buffer& operator=(Buffer&&) noexcept = default;
        ~Buffer() = default;
    private:
        std::shared_ptr<VkBuffer> buffer;
        std::shared_ptr<VkDeviceMemory> memory;

        size_t size;
    };

}

#endif // BUFFER_HPP
