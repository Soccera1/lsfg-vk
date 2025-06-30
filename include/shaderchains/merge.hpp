#ifndef MERGE_HPP
#define MERGE_HPP

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
    /// Shader chain merge.
    ///
    /// Takes the two previous frames as well as related resources
    /// and merges them into a new frame.
    ///
    class Merge {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImg1 The first frame texture
        /// @param inImg2 The second frame texture
        /// @param inImg3 The first related input texture
        /// @param inImg4 The second related input texture
        /// @param inImg5 The third related input texture
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Merge(const Device& device, const Core::DescriptorPool& pool,
            const Core::Image& inImg1,
            const Core::Image& inImg2,
            const Core::Image& inImg3,
            const Core::Image& inImg4,
            const Core::Image& inImg5);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

        /// Get the output image
        [[nodiscard]] const auto& getOutImage() const { return this->outImg; }

        /// Trivially copyable, moveable and destructible
        Merge(const Merge&) noexcept = default;
        Merge& operator=(const Merge&) noexcept = default;
        Merge(Merge&&) noexcept = default;
        Merge& operator=(Merge&&) noexcept = default;
        ~Merge() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        Core::DescriptorSet descriptorSet;
        Core::Buffer buffer;

        Core::Image inImg1;
        Core::Image inImg2;
        Core::Image inImg3;
        Core::Image inImg4;
        Core::Image inImg5;

        Core::Image outImg;
    };

}

#endif // MERGE_HPP
