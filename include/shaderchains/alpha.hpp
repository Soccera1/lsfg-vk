#ifndef ALPHA_HPP
#define ALPHA_HPP

#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/descriptorset.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"

namespace Vulkan::Shaderchains {

    ///
    /// Shader chain alpha.
    ///
    /// Takes an 8-bit R image creates four quarter-sized 8-bit RGBA images.
    ///
    class Alpha {
    public:
        ///
        /// Initialize the shaderchain.
        ///
        /// @param device The Vulkan device to create the resources on.
        /// @param pool The descriptor pool to allocate in.
        /// @param inImg The input image to process
        ///
        /// @throws ls::vulkan_error if resource creation fails.
        ///
        Alpha(const Device& device, const Core::DescriptorPool& pool,
            const Core::Image& inImg);

        ///
        /// Dispatch the shaderchain.
        ///
        /// @param buf The command buffer to use for dispatching.
        ///
        /// @throws std::logic_error if the command buffer is not recording.
        ///
        void Dispatch(const Core::CommandBuffer& buf);

        /// Get the output images.
        [[nodiscard]] const auto& getOutImages() const { return this->outImgs; }

        /// Trivially copyable, moveable and destructible
        Alpha(const Alpha&) noexcept = default;
        Alpha& operator=(const Alpha&) noexcept = default;
        Alpha(Alpha&&) noexcept = default;
        Alpha& operator=(Alpha&&) noexcept = default;
        ~Alpha() = default;
    private:
        std::vector<Core::ShaderModule> shaderModules{4};
        std::vector<Core::Pipeline> pipelines{4};
        std::vector<Core::DescriptorSet> descriptorSets{4};

        Core::Image inImg;

        std::vector<Core::Image> tempImgs1{2}; // half-size
        std::vector<Core::Image> tempImgs2{2}; // half-size
        std::vector<Core::Image> tempImgs3{4}; // quarter-size

        std::vector<Core::Image> outImgs{4}; // quarter-size
    };

}

#endif // ALPHA_HPP
