#ifndef DELTA_HPP
#define DELTA_HPP

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
    /// Shader chain delta.
    ///
    /// Takes two 8-bit RGBA images and an optional third 16-bit half-res RGBA image,
    /// producing a full-res 16-bit RGBA image.
    ///
    class Delta {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImgs The input images to process.
        /// @param optImg An optional additional input from the previous pass.
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Delta(const Device& device, const Core::DescriptorPool& pool,
            std::array<Core::Image, 2> inImgs,
            std::optional<Core::Image> optImg);

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
        Delta(const Delta&) noexcept = default;
        Delta& operator=(const Delta&) noexcept = default;
        Delta(Delta&&) noexcept = default;
        Delta& operator=(Delta&&) noexcept = default;
        ~Delta() = default;
    private:
        std::array<Core::ShaderModule, 4> shaderModules;
        std::array<Core::Pipeline, 4> pipelines;
        std::array<Core::DescriptorSet, 4> descriptorSets;
        Core::Buffer buffer;

        std::array<Core::Image, 2> inImgs;
        std::optional<Core::Image> optImg;

        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;

        Core::Image outImg;
    };

}

#endif // DELTA_HPP
