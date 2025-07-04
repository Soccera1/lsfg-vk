#ifndef MERGE_HPP
#define MERGE_HPP

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
    /// Shader chain merge.
    ///
    /// Takes the two previous frames as well as related resources
    /// and merges them into a new frame.
    ///
    class Merge {
    public:
        Merge() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param shaderpool The shader pool to use for shader modules.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImg1 The prev full image when fc % 2 == 0
        /// @param inImg2 The next full image when fc % 2 == 0
        /// @param inImg3 The first related input texture
        /// @param inImg4 The second related input texture
        /// @param inImg5 The third related input texture
        /// @param outFds File descriptors for the output images.
        /// @param genc The amount of frames to generaten.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Merge(const Core::Device& device, Pool::ShaderPool& shaderpool,
            const Core::DescriptorPool& pool,
            Core::Image inImg1,
            Core::Image inImg2,
            Core::Image inImg3,
            Core::Image inImg4,
            Core::Image inImg5,
            const std::vector<int>& outFds,
            size_t genc);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        /// @param fc The frame count, used to select the input images.
        /// @param pass The pass number.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t fc, uint64_t pass);

        /// Get the output image
        [[nodiscard]] const auto& getOutImage(size_t pass) const { return this->outImgs.at(pass); }

        /// Trivially copyable, moveable and destructible
        Merge(const Merge&) noexcept = default;
        Merge& operator=(const Merge&) noexcept = default;
        Merge(Merge&&) noexcept = default;
        Merge& operator=(Merge&&) noexcept = default;
        ~Merge() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        std::vector<std::array<Core::DescriptorSet, 2>> nDescriptorSets; // per combo
        std::vector<Core::Buffer> buffers;

        Core::Image inImg1;
        Core::Image inImg2;
        Core::Image inImg3;
        Core::Image inImg4;
        Core::Image inImg5;

        std::vector<Core::Image> outImgs;
    };

}

#endif // MERGE_HPP
