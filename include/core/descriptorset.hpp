#ifndef DESCRIPTORSET_HPP
#define DESCRIPTORSET_HPP

#include "core/buffer.hpp"
#include "core/commandbuffer.hpp"
#include "core/descriptorpool.hpp"
#include "core/image.hpp"
#include "core/pipeline.hpp"
#include "core/sampler.hpp"
#include "core/shadermodule.hpp"
#include "device.hpp"

#include <vulkan/vulkan_core.h>

#include <variant>
#include <memory>

namespace Vulkan::Core {

    ///
    /// C++ wrapper class for a Vulkan descriptor set.
    ///
    /// This class manages the lifetime of a Vulkan descriptor set.
    ///
    class DescriptorSet {
    public:
        ///
        /// Create the descriptor set.
        ///
        /// @param device Vulkan device
        /// @param pool Descriptor pool to allocate from
        /// @param shaderModule Shader module to use for the descriptor set
        ///
        /// @throws ls::vulkan_error if object creation fails.
        ///
        DescriptorSet(const Device& device,
            DescriptorPool pool, const ShaderModule& shaderModule);

        using ResourcePair = std::pair<VkDescriptorType, std::variant<Image, Sampler, Buffer>>;

        ///
        /// Update the descriptor set with resources.
        ///
        /// @param device Vulkan device
        /// @param resources Resources to update the descriptor set with
        ///
        void update(const Device& device,
            const std::vector<std::vector<ResourcePair>>& resources) const;

        ///
        /// Bind a descriptor set to a command buffer.
        ///
        /// @param commandBuffer Command buffer to bind the descriptor set to.
        /// @param pipeline Pipeline to bind the descriptor set to.
        ///
        void bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline) const;

        /// Get the Vulkan handle.
        [[nodiscard]] auto handle() const { return *this->descriptorSet; }

        /// Trivially copyable, moveable and destructible
        DescriptorSet(const DescriptorSet&) noexcept = default;
        DescriptorSet& operator=(const DescriptorSet&) noexcept = default;
        DescriptorSet(DescriptorSet&&) noexcept = default;
        DescriptorSet& operator=(DescriptorSet&&) noexcept = default;
        ~DescriptorSet() = default;
    private:
        std::shared_ptr<VkDescriptorSet> descriptorSet;
    };

}

#endif // DESCRIPTORSET_HPP
