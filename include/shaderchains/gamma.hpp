#ifndef GAMMA_HPP
#define GAMMA_HPP

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
    /// Shader chain gamma.
    ///
    /// Takes four temporal 8-bit RGBA images, as well as four output images from a given alpha stage.
    /// Also takes the corresponding (smallest if oob) output image from the beta pass.
    /// On non-first passes optionally takes 2 output images from previous gamma pass.
    /// Creates two images, one at twice the resolution of input images and the other with R16G16B16A16_FLOAT.
    ///
    class Gamma {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param temporalImgs The temporal images to use for processing.
        /// @param inImgs1 The input images to process.
        /// @param inImg2 The second input image to process, next step up the resolution.
        /// @param optImg1 An optional additional input from the previous pass.
        /// @param optImg2 An optional additional input image for processing non-first passes.
        /// @param outExtent The extent of the output image.
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Gamma(const Device& device, const Core::DescriptorPool& pool,
            const std::vector<Core::Image>& temporalImgs,
            const std::vector<Core::Image>& inImgs1,
            const Core::Image& inImg2,
            const std::optional<Core::Image>& optImg1,
            const std::optional<Core::Image>& optImg2,
            VkExtent2D outExtent);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

        /// Get the first output image.
        [[nodiscard]] const auto& getOutImage1() const { return this->outImg1; }
        /// Get the second output image.
        [[nodiscard]] const auto& getOutImage2() const { return this->outImg2; }

        /// Trivially copyable, moveable and destructible
        Gamma(const Gamma&) noexcept = default;
        Gamma& operator=(const Gamma&) noexcept = default;
        Gamma(Gamma&&) noexcept = default;
        Gamma& operator=(Gamma&&) noexcept = default;
        ~Gamma() = default;
    private:
        std::vector<Core::ShaderModule> shaderModules{6};
        std::vector<Core::Pipeline> pipelines{6};
        std::vector<Core::DescriptorSet> descriptorSets{6};
        Core::Buffer buffer;

        std::vector<Core::Image> temporalImgs{4};
        std::vector<Core::Image> inImgs1{4};
        Core::Image inImg2;
        Core::Image optImg1; // specified or created black
        std::optional<Core::Image> optImg2;

        std::vector<Core::Image> tempImgs1{4};
        std::vector<Core::Image> tempImgs2{4};
        Core::Image whiteImg;

        Core::Image outImg1;
        Core::Image outImg2;
    };

}

#endif // GAMMA_HPP
