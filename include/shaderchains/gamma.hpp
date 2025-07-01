#ifndef GAMMA_HPP
#define GAMMA_HPP

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
    /// Shader chain gamma.
    ///
    /// Takes four temporal 8-bit RGBA images, as well as four output images from a given alpha stage.
    /// Also takes the corresponding (smallest if oob) output image from the beta pass.
    /// On non-first passes optionally takes 2 output images from previous gamma pass.
    /// Creates two images, one at twice the resolution of input images and the other with R16G16B16A16_FLOAT.
    ///
    class Gamma {
    public:
        Gamma() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImgs1_0 The next input images to process (when fc % 3 == 0).
        /// @param inImgs1_1 The prev input images to process (when fc % 3 == 0).
        /// @param inImgs1_2 Initially unprocessed prev prev input images (when fc % 3 == 0).
        /// @param inImg2 The second input image to process, next step up the resolution.
        /// @param optImg1 An optional additional input from the previous pass.
        /// @param optImg2 An optional additional input image for processing non-first passes.
        /// @param outExtent The extent of the output image.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Gamma(const Core::Device& device, const Core::DescriptorPool& pool,
            std::array<Core::Image, 4> inImgs1_0,
            std::array<Core::Image, 4> inImgs1_1,
            std::array<Core::Image, 4> inImgs1_2,
            Core::Image inImg2,
            std::optional<Core::Image> optImg1,
            std::optional<Core::Image> optImg2,
            VkExtent2D outExtent);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        /// @param fc The frame count, used to select the input images.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t fc);

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
        std::array<Core::ShaderModule, 6> shaderModules;
        std::array<Core::Pipeline, 6> pipelines;
        std::array<Core::DescriptorSet, 5> descriptorSets; // first shader has special logic
        std::array<Core::DescriptorSet, 3> specialDescriptorSets;
        Core::Buffer buffer;

        std::array<Core::Image, 4> inImgs1_0;
        std::array<Core::Image, 4> inImgs1_1;
        std::array<Core::Image, 4> inImgs1_2;
        Core::Image inImg2;
        Core::Image optImg1; // specified or created black
        std::optional<Core::Image> optImg2;

        std::array<Core::Image, 4> tempImgs1;
        std::array<Core::Image, 4> tempImgs2;
        Core::Image whiteImg;

        Core::Image outImg1;
        Core::Image outImg2;
    };

}

#endif // GAMMA_HPP
