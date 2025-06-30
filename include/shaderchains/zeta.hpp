#ifndef ZETA_HPP
#define ZETA_HPP

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
    /// Shader chain zeta.
    ///
    /// Takes three 8-bit RGBA textures, a fourth 8-bit R texture, a fifth
    /// half-res 16-bit RGBA texture and produces a full-res 16-bit RGBA texture.
    ///
    class Zeta {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param inImgs1 The first set of input images to process.
        /// @param inImg2 The second type image to process.
        /// @param inImg3 The third type image to process.
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Zeta(const Device& device, const Core::DescriptorPool& pool,
            const std::vector<Core::Image>& inImgs1,
            const Core::Image& inImg2,
            const Core::Image& inImg3);

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
        Zeta(const Zeta&) noexcept = default;
        Zeta& operator=(const Zeta&) noexcept = default;
        Zeta(Zeta&&) noexcept = default;
        Zeta& operator=(Zeta&&) noexcept = default;
        ~Zeta() = default;
    private:
        std::vector<Core::ShaderModule> shaderModules{4};
        std::vector<Core::Pipeline> pipelines{4};
        std::vector<Core::DescriptorSet> descriptorSets{4};
        Core::Buffer buffer;

        std::vector<Core::Image> inImgs1{3};
        Core::Image inImg2;
        Core::Image inImg3;

        std::vector<Core::Image> tempImgs1{4};
        std::vector<Core::Image> tempImgs2{4};

        Core::Image outImg;
    };

}

#endif // ZETA_HPP
