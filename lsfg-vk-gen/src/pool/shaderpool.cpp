#include "pool/shaderpool.hpp"

using namespace LSFG;
using namespace LSFG::Pool;

const std::unordered_map<std::string, uint32_t> SHADERS = {
    { "downsample.spv", 0xe365474d },
    { "alpha/0.spv",    0x35f63c83 },
    { "alpha/1.spv",    0x83e5240d },
    { "alpha/2.spv",    0x5d64d9f1 },
    { "alpha/3.spv",    0xad77afe1 },
    { "beta/0.spv",     0xa986ccbb },
    { "beta/1.spv",     0x60944cf5 },
    { "beta/2.spv",     0xb1c8f69b },
    { "beta/3.spv",     0x87cbe880 },
    { "beta/4.spv",     0xc2c5507d },
    { "gamma/0.spv",    0xccce9dab },
    { "gamma/1.spv",    0x7719e229 },
    { "gamma/2.spv",    0xfb1a7643 },
    { "gamma/3.spv",    0xe0553cd8 },
    { "gamma/4.spv",    0xf73c136f },
    { "gamma/5.spv",    0xa34959c },
    { "magic.spv",      0x443ea7a1 },
    { "delta/0.spv",    0x141daaac },
    { "delta/1.spv",    0x2a0ed691 },
    { "delta/2.spv",    0x23bdc583 },
    { "delta/3.spv",    0x52bc5e0f },
    { "epsilon/0.spv",  0x128eb7d7 },
    { "epsilon/1.spv",  0xbab811ad },
    { "epsilon/2.spv",  0x1d4b902d },
    { "epsilon/3.spv",  0x91236549 },
    { "zeta/0.spv",     0x7719e229 },
    { "zeta/1.spv",     0xfb1a7643 },
    { "zeta/2.spv",     0xe0553cd8 },
    { "zeta/3.spv",     0xf73c136f },
    { "extract.spv",    0xb6cb084a },
    { "merge.spv",      0xfc0aedfa }
};

Core::ShaderModule ShaderPool::getShader(
        const Core::Device& device, const std::string& name,
        const std::vector<std::pair<size_t, VkDescriptorType>>& types) {
    auto it = shaders.find(name);
    if (it != shaders.end())
        return it->second;

    // grab the shader
    auto hit = SHADERS.find(name);
    if (hit == SHADERS.end())
        throw std::runtime_error("Shader not found: " + name);
    auto hash = hit->second;

    auto dxbc = this->extractor.getResource(hash);
    if (dxbc.empty())
        throw std::runtime_error("Shader code is empty: " + name);

    // create the translated shader module
    auto spirv = dxbcToSpirv(dxbc);
    if (spirv.empty())
        throw std::runtime_error("Shader code translation failed: " + name);

    Core::ShaderModule shader(device, spirv, types);
    shaders[name] = shader;
    return shader;
}
