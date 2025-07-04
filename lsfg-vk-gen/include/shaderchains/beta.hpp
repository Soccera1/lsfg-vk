#ifndef BETA_HPP
#define BETA_HPP

#include "pool/shaderpool.hpp"
#include "core/buffer.hpp"
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
    /// Shader chain beta.
    ///
    /// Takes eight temporal 8-bit RGBA images, as well as the four output images from alpha,
    /// and creates six 8-bit R images, halving in resolution each step.
    ///
    class Beta {
    public:
        Beta() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param shaderpool The shader pool to use for shader modules.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImgs_0 The next input images to process (when fc % 3 == 0)
        /// @param inImgs_1 The prev input images to process (when fc % 3 == 0)
        /// @param inImgs_2 The prev prev input images to process (when fc % 3 == 0)
        /// @param genc Amount of frames to generate.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Beta(const Core::Device& device, Pool::ShaderPool& shaderpool,
            const Core::DescriptorPool& pool,
            std::array<Core::Image, 4> inImgs_0,
            std::array<Core::Image, 4> inImgs_1,
            std::array<Core::Image, 4> inImgs_2,
            size_t genc);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        /// @param fc The frame count, used to select the input images.
        /// @param pass The pass number
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t fc, uint64_t pass);

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
        std::array<Core::DescriptorSet, 3> descriptorSets; // first shader has special logic
        std::array<Core::DescriptorSet, 3> specialDescriptorSets;
        std::vector<Core::DescriptorSet> nDescriptorSets;
        std::vector<Core::Buffer> buffers;

        std::array<Core::Image, 4> inImgs_0;
        std::array<Core::Image, 4> inImgs_1;
        std::array<Core::Image, 4> inImgs_2;

        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;

        std::array<Core::Image, 6> outImgs;
    };

}

#endif // BETA_HPP
