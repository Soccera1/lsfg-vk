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

    class DescriptorSetUpdateBuilder;

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

        using ResourceList = std::variant<
            std::pair<VkDescriptorType, const std::vector<Image>&>,
            std::pair<VkDescriptorType, const std::vector<Sampler>&>,
            std::pair<VkDescriptorType, const std::vector<Buffer>&>
        >;

        ///
        /// Update the descriptor set with resources.
        ///
        /// @param device Vulkan device
        ///
        [[nodiscard]] DescriptorSetUpdateBuilder update(const Device& device) const;

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

    ///
    /// Builder class for updating a descriptor set.
    ///
    class DescriptorSetUpdateBuilder {
        friend class DescriptorSet;
    public:
        /// Add a resource to the descriptor set update.
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const Image& image);
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const Sampler& sampler);
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const Buffer& buffer);

        /// Add a list of resources to the descriptor set update.
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::vector<Image>& images) {
            for (const auto& image : images) this->add(type, image); return *this; }
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::vector<Sampler>& samplers) {
            for (const auto& sampler : samplers) this->add(type, sampler); return *this; }
        DescriptorSetUpdateBuilder& add(VkDescriptorType type, const std::vector<Buffer>& buffers) {
            for (const auto& buffer : buffers) this->add(type, buffer); return *this; }

        /// Finish building the descriptor set update.
        void build() const;
    private:
        const DescriptorSet* descriptorSet;
        const Device* device;

        DescriptorSetUpdateBuilder(const DescriptorSet& descriptorSet, const Device& device)
                : descriptorSet(&descriptorSet), device(&device) {}

        std::vector<VkWriteDescriptorSet> entries;
    };

}

#endif // DESCRIPTORSET_HPP
