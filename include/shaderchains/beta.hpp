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
            const std::vector<Core::Image>& temporalImgs,
            const std::vector<Core::Image>& inImgs);

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
        std::vector<Core::ShaderModule> shaderModules{5};
        std::vector<Core::Pipeline> pipelines{5};
        std::vector<Core::DescriptorSet> descriptorSets{5};
        Core::Buffer buffer;

        std::vector<Core::Image> temporalImgs{8};
        std::vector<Core::Image> inImgs{4};

        std::vector<Core::Image> tempImgs1{2};
        std::vector<Core::Image> tempImgs2{2};

        std::vector<Core::Image> outImgs{6};
    };

}

#endif // BETA_HPP
