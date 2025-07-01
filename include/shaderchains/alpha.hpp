#ifndef ALPHA_HPP
#define ALPHA_HPP

#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "core/device.hpp"

#include <array>

namespace LSFG::Shaderchains {

    ///
    /// Shader chain alpha.
    ///
    /// Takes an 8-bit R image creates four quarter-sized 8-bit RGBA images.
    ///
    class Alpha {
    public:
        Alpha() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImg The input image to process
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Alpha(const Core::Device& device, const Core::DescriptorPool& pool,
            Core::Image inImg);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        /// @param fc The frame count, used to determine which output images to write to.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t fc);

        /// Get the output images written to when fc % 3 == 0
        [[nodiscard]] const auto& getOutImages0() const { return this->outImgs_0; }
        /// Get the output images written to when fc % 3 == 1
        [[nodiscard]] const auto& getOutImages1() const { return this->outImgs_1; }
        /// Get the output images written to when fc % 3 == 2
        [[nodiscard]] const auto& getOutImages2() const { return this->outImgs_2; }

        /// Trivially copyable, moveable and destructible
        Alpha(const Alpha&) noexcept = default;
        Alpha& operator=(const Alpha&) noexcept = default;
        Alpha(Alpha&&) noexcept = default;
        Alpha& operator=(Alpha&&) noexcept = default;
        ~Alpha() = default;
    private:
        std::array<Core::ShaderModule, 4> shaderModules;
        std::array<Core::Pipeline, 4> pipelines;
        std::array<Core::DescriptorSet, 3> descriptorSets; // last shader is special
        std::array<Core::DescriptorSet, 3> specialDescriptorSets;

        Core::Image inImg;

        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;
        std::array<Core::Image, 4> tempImgs3;

        std::array<Core::Image, 4> outImgs_0;
        std::array<Core::Image, 4> outImgs_1;
        std::array<Core::Image, 4> outImgs_2;
    };

}

#endif // ALPHA_HPP
