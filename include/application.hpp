#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <unordered_map>
#include <vector>

#include <vulkan/vulkan_core.h>

class SwapchainContext;

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

    ///
    /// Add a swapchain to the application.
    ///
    /// @param handle The Vulkan handle of the swapchain.
    /// @param format The format of the swapchain.
    /// @param extent The extent of the images.
    /// @param images The swapchain images.
    ///
    /// @throws std::invalid_argument if the handle is already added.
    /// @throws ls::vulkan_error if any Vulkan call fails.
    ///
    void addSwapchain(VkSwapchainKHR handle, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images);

    ///
    /// Remove a swapchain from the application.
    ///
    /// @param handle The Vulkan handle of the swapchain state to remove.
    /// @return true if the swapchain was removed, false if it was already retired.
    ///
    /// @throws std::invalid_argument if the handle is null.
    ///
    bool removeSwapchain(VkSwapchainKHR handle);


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
    std::unordered_map<VkSwapchainKHR, SwapchainContext> swapchains;
};

///
/// An application's Vulkan swapchain and it's associated resources.
///
class SwapchainContext {
public:

    ///
    /// Create the swapchain context.
    ///
    /// @param swapchain The Vulkan swapchain handle.
    /// @param format The format of the swapchain images.
    /// @param extent The extent of the swapchain images.
    /// @param images The swapchain images.
    ///
    /// @throws std::invalid_argument if any parameter is null
    /// @throws ls::vulkan_error if any Vulkan call fails.
    ///
    SwapchainContext(VkSwapchainKHR swapchain, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images);

    /// Get the Vulkan swapchain handle.
    [[nodiscard]] VkSwapchainKHR handle() const { return this->swapchain; }
    /// Get the format of the swapchain images.
    [[nodiscard]] VkFormat getFormat() const { return this->format; }
    /// Get the extent of the swapchain images.
    [[nodiscard]] VkExtent2D getExtent() const { return this->extent; }
    /// Get the swapchain images.
    [[nodiscard]] const std::vector<VkImage>& getImages() const { return this->images; }

    // Non-copyable, trivially moveable and destructible
    SwapchainContext(const SwapchainContext&) = delete;
    SwapchainContext& operator=(const SwapchainContext&) = delete;
    SwapchainContext(SwapchainContext&&) = default;
    SwapchainContext& operator=(SwapchainContext&&) = default;
    ~SwapchainContext() = default;
private:
    // (non-owned resources)
    VkSwapchainKHR swapchain{};
    VkFormat format{};
    VkExtent2D extent{};
    std::vector<VkImage> images;

    // (owned resources)
};

#endif // APPLICATION_HPP
