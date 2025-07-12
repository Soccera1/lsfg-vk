#pragma once

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "common/utils.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace LSFG::Shaders {

    ///
    /// Delta shader.
    ///
    class Delta {
    public:
        Delta() = default;

        ///
        /// Initialize the shaderchain.
        ///
        /// @param inImgs1 Three sets of two RGBA images, corresponding to a frame count % 3.
        /// @param inImg2 Second Input image
        /// @param optImg1 Optional image for non-first passes.
        /// @param optImg2 Second optional image for non-first passes.
        ///
        /// @throws LSFG::vulkan_error if resource creation fails.
        ///
        Delta(Vulkan& vk, std::array<std::array<Core::Image, 2>, 3> inImgs1,
            Core::Image inImg2,
            std::optional<Core::Image> optImg1,
            std::optional<Core::Image> optImg2);

        ///
        /// Dispatch the shaderchain.
        ///
        void Dispatch(const Core::CommandBuffer& buf, uint64_t frameCount, uint64_t pass_idx,
            bool last);

        /// Get the first output image
        [[nodiscard]] const auto& getOutImage1() const { return this->outImg1; }
        /// Get the second output image
        [[nodiscard]] const auto& getOutImage2() const { return this->outImg2; }

        /// Trivially copyable, moveable and destructible
        Delta(const Delta&) noexcept = default;
        Delta& operator=(const Delta&) noexcept = default;
        Delta(Delta&&) noexcept = default;
        Delta& operator=(Delta&&) noexcept = default;
        ~Delta() = default;
    private:
        std::array<Core::ShaderModule, 10> shaderModules;
        std::array<Core::Pipeline, 10> pipelines;
        std::array<Core::Sampler, 3> samplers;
        struct DeltaPass {
            Core::Buffer buffer;
            std::array<Core::DescriptorSet, 3> firstDescriptorSet;
            std::array<Core::DescriptorSet, 8> descriptorSets;
            std::array<Core::DescriptorSet, 3> sixthDescriptorSet;
        };
        std::vector<DeltaPass> passes;

        std::array<std::array<Core::Image, 2>, 3> inImgs1;
        Core::Image inImg2;
        std::optional<Core::Image> optImg1, optImg2;
        std::array<Core::Image, 3> tempImgs1;
        std::array<Core::Image, 2> tempImgs2;
        Core::Image outImg1, outImg2;
    };

}
