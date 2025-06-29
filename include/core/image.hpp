#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan image.
    ///
    /// This class manages the lifetime of a Vulkan image.
    ///
    class Image {
    public:
        ///
        /// Create the image.
        ///
        /// @param device Vulkan device
        /// @param extent Extent of the image in pixels.
        /// @param format Vulkan format of the image
        /// @param usage Usage flags for the image
        /// @param aspectFlags Aspect flags for the image view
        ///
        /// @throws std::invalid_argument if the device is invalid.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Image(const Device& device, VkExtent2D extent, VkFormat format,
            VkImageUsageFlags usage, VkImageAspectFlags aspectFlags);

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->image; }
        /// Get the Vulkan image view handle.
        [[nodiscard]] auto getView() const { return *this->view; }
        /// Get the extent of the image.
        [[nodiscard]] VkExtent2D getExtent() const { return this->extent; }
        /// Get the format of the image.
        [[nodiscard]] VkFormat getFormat() const { return this->format; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->image); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        /// Trivially copyable, moveable and destructible
        Image(const Image&) noexcept = default;
        Image& operator=(const Image&) noexcept = default;
        Image(Image&&) noexcept = default;
        Image& operator=(Image&&) noexcept = default;
        ~Image() = default;
    private:
        std::shared_ptr<VkImage> image;
        std::shared_ptr<VkDeviceMemory> memory;
        std::shared_ptr<VkImageView> view;

        VkExtent2D extent;
        VkFormat format;
    };

}

#endif // IMAGE_HPP
