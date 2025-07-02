#ifndef EXTRACT_HPP
#define EXTRACT_HPP

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "core/device.hpp"

namespace LSFG::Shaderchains {

    ///
    /// Shader chain extract.
    ///
    /// Takes two half-res 16-bit RGBA textures, producing
    /// an full-res 8-bit RGBA texture.
    ///
    class Extract {
    public:
        Extract() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImg1 The first set of input images to process.
        /// @param inImg2 The second type image to process.
        /// @param outExtent The extent of the output image.
        /// @param genc Amount of frames to generate.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Extract(const Core::Device& device, const Core::DescriptorPool& pool,
            Core::Image inImg1,
            Core::Image inImg2,
            VkExtent2D outExtent,
            size_t genc);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        /// @param pass The pass number.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t pass);

        /// Get the output image.
        [[nodiscard]] const auto& getOutImage() const { return this->outImg; }

        /// Trivially copyable, moveable and destructible
        Extract(const Extract&) noexcept = default;
        Extract& operator=(const Extract&) noexcept = default;
        Extract(Extract&&) noexcept = default;
        Extract& operator=(Extract&&) noexcept = default;
        ~Extract() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        std::vector<Core::DescriptorSet> nDescriptorSets;
        std::vector<Core::Buffer> buffers;

        Core::Image inImg1;
        Core::Image inImg2;

        Core::Image whiteImg;

        Core::Image outImg;
    };

}

#endif // EXTRACT_HPP
