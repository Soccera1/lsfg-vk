#include "application.hpp"

#include <stdexcept>

Application::Application(VkDevice device, VkPhysicalDevice physicalDevice)
        : device(device), physicalDevice(physicalDevice) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE)
        throw std::invalid_argument("Invalid Vulkan device or physical device");
}
