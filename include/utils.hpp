#ifndef UTILS_HPP
#define UTILS_HPP

#include "core/commandbuffer.hpp"
#include "core/image.hpp"
#include "core/sampler.hpp"
#include "device.hpp"

#include <stdexcept>
#include <string>
#include <array>

namespace LSFG::Utils {

    ///
    /// Insert memory barriers for images in a command buffer.
    ///
    /// @throws std::logic_error if the command buffer is not in Recording state
    ///
    class BarrierBuilder {
    public:
        /// Create a barrier builder.
        BarrierBuilder(const Core::CommandBuffer& buffer)
                : commandBuffer(&buffer) {
            this->barriers.reserve(16); // this is performance critical
        }

        // Add a resource to the barrier builder.
        BarrierBuilder& addR2W(Core::Image& image);
        BarrierBuilder& addW2R(Core::Image& image);

        // Add an optional resource to the barrier builder.
        BarrierBuilder& addR2W(std::optional<Core::Image>& image) {
            if (image.has_value()) this->addR2W(*image); return *this; }
        BarrierBuilder& addW2R(std::optional<Core::Image>& image) {
            if (image.has_value()) this->addW2R(*image); return *this; }

        /// Add a list of resources to the barrier builder.
        BarrierBuilder& addR2W(std::vector<Core::Image>& images) {
            for (auto& image : images) this->addR2W(image); return *this; }
        BarrierBuilder& addW2R(std::vector<Core::Image>& images) {
            for (auto& image : images) this->addW2R(image); return *this; }

        /// Add an array of resources to the barrier builder.
        template<std::size_t N>
        BarrierBuilder& addR2W(std::array<Core::Image, N>& images) {
            for (auto& image : images) this->addR2W(image); return *this; }
        template<std::size_t N>
        BarrierBuilder& addW2R(std::array<Core::Image, N>& images) {
            for (auto& image : images) this->addW2R(image); return *this; }

        /// Finish building the barrier
        void build() const;
    private:
        const Core::CommandBuffer* commandBuffer;

        std::vector<VkImageMemoryBarrier2> barriers;
    };

    ///
    /// Upload a DDS file to a Vulkan image.
    ///
    /// @param device The Vulkan device
    /// @param commandPool The command pool
    /// @param image The Vulkan image to upload to
    /// @param path The path to the DDS file.
    ///
    /// @throws std::system_error If the file cannot be opened or read.
    /// @throws ls:vulkan_error If the Vulkan image cannot be created or updated.
    ///
    void uploadImage(const Device& device,
        const Core::CommandPool& commandPool,
        Core::Image& image, const std::string& path);

    ///
    /// Clear a texture to white during setup.
    ///
    /// @param device The Vulkan device.
    /// @param image The image to clear.
    /// @param white If true, the image will be cleared to white, otherwise to black.
    ///
    /// @throws LSFG::vulkan_error If the Vulkan image cannot be cleared.
    ///
    void clearImage(const Device& device, Core::Image& image, bool white = false);

}

namespace LSFG::Globals {

    /// Global sampler with address mode set to clamp to border.
    extern Core::Sampler samplerClampBorder;
    /// Global sampler with address mode set to clamp to edge.
    extern Core::Sampler samplerClampEdge;

    /// Commonly used constant buffer structure for shaders.
    struct FgBuffer {
        std::array<uint32_t, 2> inputOffset;
        uint32_t firstIter;
        uint32_t firstIterS;
        uint32_t advancedColorKind;
        uint32_t hdrSupport;
        float resolutionInvScale;
        float timestamp;
        float uiThreshold;
        std::array<uint32_t, 3> pad;
    };

    /// Default instance of the FgBuffer.
    extern FgBuffer fgBuffer;

    static_assert(sizeof(FgBuffer) == 48, "FgBuffer must be 48 bytes in size.");

    /// Initialize global resources.
    void initializeGlobals(const Device& device);

    /// Uninitialize global resources.
    void uninitializeGlobals() noexcept;

}

#endif // UTILS_HPP
