#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "mini/image.hpp"
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
    /// @param graphicsQueue Vulkan queue for graphics operations
    /// @param presentQueue Vulkan queue for presentation operations
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    Application(VkDevice device, VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue, VkQueue presentQueue);

    ///
    /// Add a swapchain to the application.
    ///
    /// @param handle The Vulkan handle of the swapchain.
    /// @param format The format of the swapchain.
    /// @param extent The extent of the images.
    /// @param images The swapchain images.
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    void addSwapchain(VkSwapchainKHR handle, VkFormat format, VkExtent2D extent,
        const std::vector<VkImage>& images);

    ///
    /// Present the next frame on a given swapchain.
    ///
    /// @param handle The Vulkan handle of the swapchain to present on.
    /// @param queue The Vulkan queue to present the frame on.
    /// @param semaphores The semaphores to wait on before presenting.
    /// @param idx The index of the swapchain image to present.
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    void presentSwapchain(VkSwapchainKHR handle, VkQueue queue,
        const std::vector<VkSemaphore>& semaphores, uint32_t idx);

    ///
    /// Remove a swapchain from the application.
    ///
    /// @param handle The Vulkan handle of the swapchain state to remove.
    /// @return true if the swapchain was removed, false if it was already retired.
    ///
    bool removeSwapchain(VkSwapchainKHR handle);

    /// Get the Vulkan device.
    [[nodiscard]] VkDevice getDevice() const { return this->device; }
    /// Get the Vulkan physical device.
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return this->physicalDevice; }
    /// Get the Vulkan graphics queue.
    [[nodiscard]] VkQueue getGraphicsQueue() const { return this->graphicsQueue; }
    /// Get the Vulkan present queue.
    [[nodiscard]] VkQueue getPresentQueue() const { return this->presentQueue; }

    // Non-copyable and non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /// Destructor, cleans up resources.
    ~Application();
private:
    // (non-owned resources)
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

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
    /// @param app The application context to use.
    /// @param swapchain The Vulkan swapchain handle.
    /// @param format The format of the swapchain images.
    /// @param extent The extent of the swapchain images.
    /// @param images The swapchain images.
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    SwapchainContext(const Application& app, VkSwapchainKHR swapchain,
        VkFormat format, VkExtent2D extent, const std::vector<VkImage>& images);

    ///
    /// Present the next frame
    ///
    /// @param app The application context to use
    /// @param queue The Vulkan queue to present the frame on.
    /// @param semaphores The semaphores to wait on before presenting.
    /// @param idx The index of the swapchain image to present.
    ///
    /// @throws LSFG::vulkan_error if any Vulkan call fails.
    ///
    void present(const Application& app, VkQueue queue,
        const std::vector<VkSemaphore>& semaphores, uint32_t idx);

    /// Get the Vulkan swapchain handle.
    [[nodiscard]] VkSwapchainKHR handle() const { return this->swapchain; }
    /// Get the format of the swapchain images.
    [[nodiscard]] VkFormat getFormat() const { return this->format; }
    /// Get the extent of the swapchain images.
    [[nodiscard]] VkExtent2D getExtent() const { return this->extent; }
    /// Get the swapchain images.
    [[nodiscard]] const std::vector<VkImage>& getImages() const { return this->images; }

    // Non-copyable, trivially moveable
    SwapchainContext(const SwapchainContext&) = delete;
    SwapchainContext& operator=(const SwapchainContext&) = delete;
    SwapchainContext(SwapchainContext&&) = default;
    SwapchainContext& operator=(SwapchainContext&&) = default;

    /// Destructor, cleans up resources.
    ~SwapchainContext();
private:
    // (non-owned resources)
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;

    // (owned resources)
    Mini::Image frame_0, frame_1;
    int32_t lsfgId;
};

#endif // APPLICATION_HPP
