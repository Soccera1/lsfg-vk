#ifndef EPSILON_HPP
#define EPSILON_HPP

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
    /// Shader chain epsilon.
    ///
    /// Takes three 8-bit RGBA textures, a fourth 8-bit R texture, an optional fifth
    /// half-res 16-bit RGBA texture and produces a full-res 16-bit RGBA texture.
    ///
    class Epsilon {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImgs1 The first set of input images to process.
        /// @param inImg2 The second type image to process.
        /// @param optImg An optional additional input from the previous pass.
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Epsilon(const Device& device, const Core::DescriptorPool& pool,
            const std::vector<Core::Image>& inImgs1,
            const Core::Image& inImg2,
            const std::optional<Core::Image>& optImg);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

        /// Get the output image.
        [[nodiscard]] const auto& getOutImage() const { return this->outImg; }

        /// Trivially copyable, moveable and destructible
        Epsilon(const Epsilon&) noexcept = default;
        Epsilon& operator=(const Epsilon&) noexcept = default;
        Epsilon(Epsilon&&) noexcept = default;
        Epsilon& operator=(Epsilon&&) noexcept = default;
        ~Epsilon() = default;
    private:
        std::vector<Core::ShaderModule> shaderModules{4};
        std::vector<Core::Pipeline> pipelines{4};
        std::vector<Core::DescriptorSet> descriptorSets{4};
        Core::Buffer buffer;

        std::vector<Core::Image> inImgs1{3};
        Core::Image inImg2;
        std::optional<Core::Image> optImg;

        std::vector<Core::Image> tempImgs1{4};
        std::vector<Core::Image> tempImgs2{4};

        Core::Image outImg;
    };

}

#endif // EPSILON_HPP
