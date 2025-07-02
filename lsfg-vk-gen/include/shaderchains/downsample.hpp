#ifndef DOWNSAMPLE_HPP
#define DOWNSAMPLE_HPP

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
    /// Downsample shader.
    ///
    /// Takes an 8-bit RGBA image and downsamples it into 7x 8-bit R images.
    ///
    class Downsample {
    public:
        Downsample() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImg_0 The next full image to downsample (when fc % 2 == 0)
        /// @param inImg_1 The next full image to downsample (when fc % 2 == 1)
        /// @param genc Amount of frames to generate.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Downsample(const Core::Device& device, const Core::DescriptorPool& pool,
            Core::Image inImg_0, Core::Image inImg_1,
            size_t genc);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        /// @param fc The frame count, used to select the input image.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t fc);

        /// Get the output images.
        [[nodiscard]] const auto& getOutImages() const { return this->outImgs; }

        /// Trivially copyable, moveable and destructible
        Downsample(const Downsample&) noexcept = default;
        Downsample& operator=(const Downsample&) noexcept = default;
        Downsample(Downsample&&) noexcept = default;
        Downsample& operator=(Downsample&&) noexcept = default;
        ~Downsample() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        std::array<Core::DescriptorSet, 2> descriptorSets; // one for each input image
        Core::Buffer buffer;

        Core::Image inImg_0, inImg_1;

        std::array<Core::Image, 7> outImgs;
    };

}

#endif // DOWNSAMPLE_HPP
