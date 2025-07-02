#ifndef DELTA_HPP
#define DELTA_HPP

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
    /// Shader chain delta.
    ///
    /// Takes two 8-bit RGBA images and an optional third 16-bit half-res RGBA image,
    /// producing a full-res 16-bit RGBA image.
    ///
    class Delta {
    public:
        Delta() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImgs The input images to process.
        /// @param optImg An optional additional input from the previous pass.
        /// @param genc Amount of frames to generate.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Delta(const Core::Device& device, const Core::DescriptorPool& pool,
            std::array<Core::Image, 2> inImgs,
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
        Delta(const Delta&) noexcept = default;
        Delta& operator=(const Delta&) noexcept = default;
        Delta(Delta&&) noexcept = default;
        Delta& operator=(Delta&&) noexcept = default;
        ~Delta() = default;
    private:
        std::array<Core::ShaderModule, 4> shaderModules;
        std::array<Core::Pipeline, 4> pipelines;
        std::array<Core::DescriptorSet, 3> descriptorSets;
        std::vector<Core::DescriptorSet> nDescriptorSets;
        std::vector<Core::Buffer> buffers;

        std::array<Core::Image, 2> inImgs;
        std::optional<Core::Image> optImg;

        std::array<Core::Image, 2> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;

        Core::Image outImg;
    };

}

#endif // DELTA_HPP
