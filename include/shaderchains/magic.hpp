#ifndef MAGIC_HPP
#define MAGIC_HPP

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"

#include <array>

namespace Vulkan::Shaderchains {

    ///
    /// Shader chain magic.
    ///
    /// Takes textures similar to gamma shader chain, produces intermediary
    /// results in groups of 3, 2, 2.
    ///
    class Magic {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to use for descriptor sets.
        /// @param temporalImgs The temporal images to use for processing.
        /// @param inImgs1 The first set of input images to process.
        /// @param inImg2 The second input image to process.
        /// @param inImg3 The third input image to process, next step up the resolution.
        /// @param optImg An optional additional input from the previous pass.
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Magic(const Device& device, const Core::DescriptorPool& pool,
            std::array<Core::Image, 4> temporalImgs,
            std::array<Core::Image, 4> inImgs1,
            Core::Image inImg2,
            Core::Image inImg3,
            std::optional<Core::Image> optImg);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

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
        Core::DescriptorSet descriptorSet;
        Core::Buffer buffer;

        std::array<Core::Image, 4> temporalImgs;
        std::array<Core::Image, 4> inImgs1;
        Core::Image inImg2;
        Core::Image inImg3;
        std::optional<Core::Image> optImg;

        std::array<Core::Image, 2> outImgs1;
        std::array<Core::Image, 3> outImgs2;
        std::array<Core::Image, 3> outImgs3;
    };

}

#endif // MAGIC_HPP
