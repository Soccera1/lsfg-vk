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
            const std::vector<Core::Image>& inImgs,
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
        Delta(const Delta&) noexcept = default;
        Delta& operator=(const Delta&) noexcept = default;
        Delta(Delta&&) noexcept = default;
        Delta& operator=(Delta&&) noexcept = default;
        ~Delta() = default;
    private:
        std::vector<Core::ShaderModule> shaderModules{4};
        std::vector<Core::Pipeline> pipelines{4};
        std::vector<Core::DescriptorSet> descriptorSets{4};
        Core::Buffer buffer;

        std::vector<Core::Image> inImg{2};
        std::optional<Core::Image> optImg;

        std::vector<Core::Image> tempImgs1{2};
        std::vector<Core::Image> tempImgs2{2};

        Core::Image outImg;
    };

}

#endif // DELTA_HPP
