#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan {

    ///
    /// C++ wrapper class for a Vulkan instance.
    ///
    /// This class manages the lifetime of a Vulkan instance.
    ///
    class Instance {
    public:
        ///
        /// Create the instance.
        ///
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Instance();

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return this->instance ? *this->instance : VK_NULL_HANDLE; }

        /// Trivially copyable, moveable and destructible
        Instance(const Instance&) noexcept = default;
        Instance& operator=(const Instance&) noexcept = default;
        Instance(Instance&&) noexcept = default;
        Instance& operator=(Instance&&) noexcept = default;
        ~Instance() = default;
    private:
        std::shared_ptr<VkInstance> instance;
    };

}

#endif // INSTANCE_HPP
