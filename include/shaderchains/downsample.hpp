#ifndef DOWNSAMPLE_HPP
#define DOWNSAMPLE_HPP

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
    /// Downsample shader.
    ///
    /// Takes an 8-bit RGBA texture and downsamples it into 7x 8-bit R textures.
    ///
    class Downsample {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImage The input image to downsample.
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Downsample(const Device& device, const Core::DescriptorPool& pool,
            const Core::Image& inImage);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

        /// Get the output images.
        [[nodiscard]] const auto& getOutImages() const { return this->outImages; }

        /// Trivially copyable, moveable and destructible
        Downsample(const Downsample&) noexcept = default;
        Downsample& operator=(const Downsample&) noexcept = default;
        Downsample(Downsample&&) noexcept = default;
        Downsample& operator=(Downsample&&) noexcept = default;
        ~Downsample() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        Core::DescriptorSet descriptorSet;
        Core::Buffer buffer;

        Core::Image inImage;

        std::vector<Core::Image> outImages{7};
    };

}

#endif // DOWNSAMPLE_HPP
