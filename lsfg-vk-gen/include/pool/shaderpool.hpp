#ifndef SHADERPOOL_HPP
#define SHADERPOOL_HPP

#include "core/device.hpp"
#include "core/pipeline.hpp"
#include "core/shadermodule.hpp"
#include "pool/extract.hpp"

#include <string>
#include <unordered_map>

namespace LSFG::Pool {

    ///
    /// Shader pool for each Vulkan device.
    ///
    class ShaderPool {
    public:
        ShaderPool() noexcept = default;

        ///
        /// Create the shader pool.
        ///
        /// @param path Path to the shader dll
        ///
        /// @throws std::runtime_error if the shader pool cannot be created.
        ///
        ShaderPool(const std::string& path) : extractor(path) {}

        ///
        /// Retrieve a shader module by name or create it.
        ///
        /// @param device Vulkan device
        /// @param name Name of the shader module
        /// @param types Descriptor types for the shader module
        /// @return Shader module or empty
        ///
        /// @throws LSFG::vulkan_error if the shader module cannot be created.
        ///
        Core::ShaderModule getShader(
            const Core::Device& device, const std::string& name,
            const std::vector<std::pair<size_t, VkDescriptorType>>& types);

        ///
        /// Retrieve a pipeline shader module by name or create it.
        ///
        /// @param device Vulkan device
        /// @param name Name of the shader module
        /// @return Pipeline shader module or empty
        ///
        /// @throws LSFG::vulkan_error if the shader module cannot be created.
        ///
        Core::Pipeline getPipeline(
            const Core::Device& device, const std::string& name);
    private:
        Extractor extractor;
        std::unordered_map<std::string, Core::ShaderModule> shaders;
        std::unordered_map<std::string, Core::Pipeline> pipelines;
    };

}

#endif // SHADERPOOL_HPP
