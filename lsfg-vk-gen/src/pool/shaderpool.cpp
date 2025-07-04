#include "pool/shaderpool.hpp"

#include "pool/extract.hpp"

using namespace LSFG;
using namespace LSFG::Pool;

const std::unordered_map<std::string, std::string> SHADERS = {
    { "alpha/0.spv", "lilEUr7nBgA8P6VSqCms09t9b+DZH6dVlcefVuFHlc8=" },
    { "alpha/1.spv", "2TRNidol3BNs/aeLl2/Om7z8bAlpehkOPVtmMao1q84=" },
    { "alpha/2.spv", "tP6qIJZhd4pGr1pop1e9ztW1gwp97ufQa2GaBZBYZJE=" },
    { "alpha/3.spv", "gA4ZejNp+RwtqjtTzGdGf5D/CjSGlwFB2nOgDAIv91k=" },
    { "beta/0.spv", "uQ/xsBMKRuJhbxstBukWMhXYuppPAYygxkb/3kNu4vI=" },
    { "beta/1.spv", "BUbrL9fZREXLlg1lmlTYD6n8DwpzHkho5bI3RLbfNJg=" },
    { "beta/2.spv", "bz0lxQjMYp6HLqw12X3jfV7H0SOZKrqUhgtw17WgTx4=" },
    { "beta/3.spv", "JA5/8p7yiiiCxmuiTsOR9Fb/z1qp8KlyU2wo9Wfpbcc=" },
    { "beta/4.spv", "/I+iYEwzOFylXZJWWNQ/oUT6SeLVnpotNXGV8y/FUVk=" },
    { "delta/0.spv", "gtBWy1WtP8NO+Z1sSPMgOJ75NaPEKvthc7imNGzJkGI=" },
    { "delta/1.spv", "JiqZZIoHay/uS1ptzlz3EWKUPct/jQHoFtN0qlEtVUU=" },
    { "delta/2.spv", "zkBa37GvAG8izeIv4o/3OowpxiobfOdNmPyVWl2BSWY=" },
    { "delta/3.spv", "neIMl/PCPonXqjtZykMb9tR4yW7JkZfMTqZPGOmJQUg=" },
    { "downsample.spv", "F9BppS+ytDjO3aoMEY7deMzLaSUhX8EuI+eH8654Fpw=" },
    { "epsilon/0.spv", "YHECg9LrgTCM8lABFOXkC5qTKoHsIMWnZ6ST3dexD08=" },
    { "epsilon/1.spv", "Uv7CfTi6x69c9Exuc16UqA7fvLTUGicHZVi5jhHKo0w=" },
    { "epsilon/2.spv", "0vmxxGRa6gbl5dqmKTBO9x/ZM2oEUJ5JtGfqcHhvouQ=" },
    { "epsilon/3.spv", "Sa/gkbCCDyCyUh8BSOa882t4qDc51oeP6+Kj3O3EaxM=" },
    { "extract.spv", "xKUdoEwFJDsc/kX/aY1FyzlMlOaJX4iHQLlthe2MvBs=" },
    { "gamma/0.spv", "AJuuF/X9NnypgBd89GbXMKcXC2meysYayiZQZwu3WhU=" },
    { "gamma/1.spv", "LLr/3D+whLd6XuKkBS7rlaNN+r8qB/Khr4ii+M7KSxY=" },
    { "gamma/2.spv", "KjHXdawBR8AMK7Kud/vXJmJTddXFKppREEpsykjwZDc=" },
    { "gamma/3.spv", "zAnAC73i76AJjv0o1To3bBu2jnIWXzX3NlSMvU3Lgxw=" },
    { "gamma/4.spv", "ivQ7ltprazBOXb46yxul9HJ5ByJk2LbG034cC6NkEpk=" },
    { "gamma/5.spv", "lHYgyCpWnMIB74HL22BKQyoqUGvUjgR79W4vXFXzXe4=" },
    { "magic.spv", "ZdoTjEhrlbAxq0MtaJyk6jQ5+hrySEsnvn+jseukAuI=" },
    { "merge.spv", "dnluf4IHKNaqz6WvH7qodn+fZ56ORx+w3MUOwH7huok=" },
    { "zeta/0.spv", "LLr/3D+whLd6XuKkBS7rlaNN+r8qB/Khr4ii+M7KSxY=" },
    { "zeta/1.spv", "KjHXdawBR8AMK7Kud/vXJmJTddXFKppREEpsykjwZDc=" },
    { "zeta/2.spv", "zAnAC73i76AJjv0o1To3bBu2jnIWXzX3NlSMvU3Lgxw=" },
    { "zeta/3.spv", "ivQ7ltprazBOXb46yxul9HJ5ByJk2LbG034cC6NkEpk=" },
    { "zeta/4.spv", "lHYgyCpWnMIB74HL22BKQyoqUGvUjgR79W4vXFXzXe4=" }
};

ShaderPool::ShaderPool(const std::string& path) {
    const Extractor extractor(path);
    const Translator translator;

    for (const auto& [name, hash] : SHADERS) {
        auto data = extractor.getResource(hash);
        if (data.empty())
            throw std::runtime_error("Shader code is empty: " + name);

        auto code = translator.translate(data);
        if (code.empty())
            throw std::runtime_error("Shader code translation failed: " + name);

        shaderBytecodes[name] = std::move(code);
    }
}

Core::ShaderModule ShaderPool::getShader(
        const Core::Device& device, const std::string& name,
        const std::vector<std::pair<size_t, VkDescriptorType>>& types) {
    auto it = shaders.find(name);
    if (it != shaders.end())
        return it->second;

    // create the shader module
    auto cit = shaderBytecodes.find(name);
    if (cit == shaderBytecodes.end())
        throw std::runtime_error("Shader code translation failed: " + name);
    auto code = cit->second;

    Core::ShaderModule shader(device, code, types);
    shaders[name] = shader;
    return shader;
}
