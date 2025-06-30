#ifndef BETA_HPP
#define BETA_HPP

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"

#include <array>

namespace Vulkan::Shaderchains {

    ///
    /// Shader chain beta.
    ///
    /// Takes eight temporal 8-bit RGBA images, as well as the four output images from alpha,
    /// and creates six 8-bit R images, halving in resolution each step.
    ///
    class Beta {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param temporalImgs The temporal images to use for processing.
        /// @param inImgs The input images to process
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Beta(const Device& device, const Core::DescriptorPool& pool,
            std::array<Core::Image, 8> temporalImgs,
            std::array<Core::Image, 4> inImgs);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

        /// Get the output images.
        [[nodiscard]] const auto& getOutImages() const { return this->outImgs; }

        /// Trivially copyable, moveable and destructible
        Beta(const Beta&) noexcept = default;
        Beta& operator=(const Beta&) noexcept = default;
        Beta(Beta&&) noexcept = default;
        Beta& operator=(Beta&&) noexcept = default;
        ~Beta() = default;
    private:
        std::array<Core::ShaderModule, 5> shaderModules;
        std::array<Core::Pipeline, 5> pipelines;
        std::array<Core::DescriptorSet, 5> descriptorSets;
        Core::Buffer buffer;

        std::array<Core::Image, 8> temporalImgs;
        std::array<Core::Image, 4> inImgs;

        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;

        std::array<Core::Image, 6> outImgs;
    };

}

#endif // BETA_HPP
