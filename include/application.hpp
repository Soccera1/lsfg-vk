#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <vulkan/vulkan_core.h>

///
/// Main application class, wrapping around the Vulkan device.
///
class Application {
public:

    ///
    /// Create the application.
    ///
    /// @param device Vulkan device
    /// @param physicalDevice Vulkan physical device
    ///
    /// @throws std::invalid_argument if the device or physicalDevice is null.
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    Application(VkDevice device, VkPhysicalDevice physicalDevice);

    /// Get the Vulkan device.
    [[nodiscard]] VkDevice getDevice() const { return this->device; }
    /// Get the Vulkan physical device.
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return this->physicalDevice; }

    // Non-copyable and non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /// Destructor, cleans up resources.
    ~Application() = default; // no resources to clean up as of right now.
private:
    // (non-owned resources)
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    // (owned resources)

};

#endif // APPLICATION_HPP
