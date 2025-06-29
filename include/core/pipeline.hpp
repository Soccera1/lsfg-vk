#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "core/commandbuffer.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan pipeline.
    ///
    /// This class manages the lifetime of a Vulkan pipeline.
    ///
    class Pipeline {
    public:
        ///
        /// Create a compute pipeline.
        ///
        /// @param device Vulkan device
        /// @param shader Shader module to use for the pipeline.
        ///
        /// @throws std::invalid_argument if the device is invalid.
        /// @throws ls::vulkan_error if object creation fails.
        ///
        Pipeline(const Device& device, const ShaderModule& shader);

        ///
        /// Bind the pipeline to a command buffer.
        ///
        /// @param commandBuffer Command buffer to bind the pipeline to.
        ///
        /// @throws std::invalid_argument if the command buffer is invalid.
        ///
        void bind(const CommandBuffer& commandBuffer) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->pipeline; }
        /// Get the pipeline layout.
        [[nodiscard]] auto getLayout() const { return *this->layout; }

        /// Check whether the object is valid.
        [[nodiscard]] bool isValid() const { return static_cast<bool>(this->pipeline); }
        /// if (obj) operator. Checks if the object is valid.
        explicit operator bool() const { return this->isValid(); }

        /// Trivially copyable, moveable and destructible
        Pipeline(const Pipeline&) noexcept = default;
        Pipeline& operator=(const Pipeline&) noexcept = default;
        Pipeline(Pipeline&&) noexcept = default;
        Pipeline& operator=(Pipeline&&) noexcept = default;
        ~Pipeline() = default;
    private:
        std::shared_ptr<VkPipeline> pipeline;
        std::shared_ptr<VkPipelineLayout> layout;
    };

}

#endif // PIPELINE_HPP
