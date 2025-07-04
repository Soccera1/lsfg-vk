#ifndef MAGIC_HPP
#define MAGIC_HPP

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
    /// Shader chain magic.
    ///
    /// Takes textures similar to gamma shader chain, produces intermediary
    /// results in groups of 3, 2, 2.
    ///
    class Magic {
    public:
        Magic() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param shaderpool The shader pool to use for shader modules.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImgs1_0 The next input images to process (when fc % 3 == 0).
        /// @param inImgs1_1 The prev input images to process (when fc % 3 == 0).
        /// @param inImgs1_2 Initially unprocessed prev prev input images (when fc % 3 == 0).
        /// @param inImg2 The second input image to process.
        /// @param inImg3 The third input image to process, next step up the resolution.
        /// @param optImg An optional additional input from the previous pass.
        /// @param genc Amount of frames to generate.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Magic(const Core::Device& device, Pool::ShaderPool& shaderpool,
            const Core::DescriptorPool& pool,
            std::array<Core::Image, 4> inImgs1_0,
            std::array<Core::Image, 4> inImgs1_1,
            std::array<Core::Image, 4> inImgs1_2,
            Core::Image inImg2,
            Core::Image inImg3,
            std::optional<Core::Image> optImg,
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

        /// Get the first set of output images
        [[nodiscard]] const auto& getOutImages1() const { return this->outImgs1; }
        /// Get the second set of output images
        [[nodiscard]] const auto& getOutImages2() const { return this->outImgs2; }
        /// Get the third set of output images
        [[nodiscard]] const auto& getOutImages3() const { return this->outImgs3; }

        /// Trivially copyable, moveable and destructible
        Magic(const Magic&) noexcept = default;
        Magic& operator=(const Magic&) noexcept = default;
        Magic(Magic&&) noexcept = default;
        Magic& operator=(Magic&&) noexcept = default;
        ~Magic() = default;
    private:
        Core::ShaderModule shaderModule;
        Core::Pipeline pipeline;
        std::vector<std::array<Core::DescriptorSet, 3>> nDescriptorSets;
        std::vector<Core::Buffer> buffers;

        std::array<Core::Image, 4> inImgs1_0;
        std::array<Core::Image, 4> inImgs1_1;
        std::array<Core::Image, 4> inImgs1_2;
        Core::Image inImg2;
        Core::Image inImg3;
        std::optional<Core::Image> optImg;

        std::array<Core::Image, 2> outImgs1;
        std::array<Core::Image, 3> outImgs2;
        std::array<Core::Image, 3> outImgs3;
    };

}

#endif // MAGIC_HPP
