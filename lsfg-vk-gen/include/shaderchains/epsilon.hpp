#ifndef EPSILON_HPP
#define EPSILON_HPP

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
    /// Shader chain epsilon.
    ///
    /// Takes three 8-bit RGBA textures, a fourth 8-bit R texture, an optional fifth
    /// half-res 16-bit RGBA texture and produces a full-res 16-bit RGBA texture.
    ///
    class Epsilon {
    public:
        Epsilon() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImgs1 The first set of input images to process.
        /// @param inImg2 The second type image to process.
        /// @param optImg An optional additional input from the previous pass.
        /// @param genc Amount of frames to generate.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Epsilon(const Core::Device& device, const Core::DescriptorPool& pool,
            std::array<Core::Image, 3> inImgs1,
            Core::Image inImg2,
            std::optional<Core::Image> optImg,
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
        Epsilon(const Epsilon&) noexcept = default;
        Epsilon& operator=(const Epsilon&) noexcept = default;
        Epsilon(Epsilon&&) noexcept = default;
        Epsilon& operator=(Epsilon&&) noexcept = default;
        ~Epsilon() = default;
    private:
        std::array<Core::ShaderModule, 4> shaderModules;
        std::array<Core::Pipeline, 4> pipelines;
        std::array<Core::DescriptorSet, 3> descriptorSets;
        std::vector<Core::DescriptorSet> nDescriptorSets;
        std::vector<Core::Buffer> buffers;

        std::array<Core::Image, 3> inImgs1;
        Core::Image inImg2;
        std::optional<Core::Image> optImg;

        std::array<Core::Image, 4> tempImgs1;
        std::array<Core::Image, 4> tempImgs2;

        Core::Image outImg;
    };

}

#endif // EPSILON_HPP
