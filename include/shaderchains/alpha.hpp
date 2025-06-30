#ifndef ALPHA_HPP
#define ALPHA_HPP

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
    /// Shader chain alpha.
    ///
    /// Takes an 8-bit R image creates four quarter-sized 8-bit RGBA images.
    ///
    class Alpha {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImg The input image to process
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Alpha(const Device& device, const Core::DescriptorPool& pool,
            Core::Image inImg);

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
        Alpha(const Alpha&) noexcept = default;
        Alpha& operator=(const Alpha&) noexcept = default;
        Alpha(Alpha&&) noexcept = default;
        Alpha& operator=(Alpha&&) noexcept = default;
        ~Alpha() = default;
    private:
        std::array<Core::ShaderModule, 4> shaderModules;
        std::array<Core::Pipeline, 4> pipelines;
        std::array<Core::DescriptorSet, 4> descriptorSets;

        Core::Image inImg;

        std::array<Core::Image, 2> tempImgs1; // half-size
        std::array<Core::Image, 2> tempImgs2; // half-size
        std::array<Core::Image, 4> tempImgs3; // quarter-size

        std::array<Core::Image, 4> outImgs; // quarter-size
    };

}

#endif // ALPHA_HPP
