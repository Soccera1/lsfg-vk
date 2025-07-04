#include "pool/extract.hpp"

#include <dxbc/dxbc_modinfo.h>
#include <dxbc/dxbc_module.h>
#include <dxbc/dxbc_reader.h>
#include <dxvk/dxvk_compute.h>
#include <dxvk/dxvk_context.h>
#include <dxvk/dxvk_pipelayout.h>
#include <dxvk/dxvk_shader.h>
#include <pe-parse/parse.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <array>
#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace LSFG;
using namespace LSFG::Pool;

namespace {

    using ResourceMap = std::unordered_map<std::string, std::vector<uint8_t>>;

    /// Callback function for each resource.
    int on_resource(void* data, const peparse::resource& res) {
        if (res.type != peparse::RT_RCDATA || res.buf == nullptr || res.buf->bufLen <= 0)
            return 0;

        // hash the resource data
        std::array<uint8_t, SHA256_DIGEST_LENGTH> hash{};
        SHA256(res.buf->buf, res.buf->bufLen, hash.data());

        std::array<uint8_t, SHA256_DIGEST_LENGTH * 2 + 1> base64{};
        const int base64_len = EVP_EncodeBlock(base64.data(), hash.data(), hash.size());
        const std::string hash_str(reinterpret_cast<const char*>(base64.data()),
            static_cast<size_t>(base64_len));

        // store the resource
        std::vector<uint8_t> resource_data(res.buf->bufLen);
        std::copy_n(res.buf->buf, res.buf->bufLen, resource_data.data());

        auto* map = reinterpret_cast<ResourceMap*>(data);
        (*map)[hash_str] = resource_data;

        return 0;
    }

}

Extractor::Extractor(const std::string& path) {
    peparse::parsed_pe* pe = peparse::ParsePEFromFile(path.c_str());
    if (!pe)
        throw std::runtime_error("Unable to parse PE file: " + path);

    peparse::IterRsrc(pe, on_resource, &this->resources);
    peparse::DestructParsedPE(pe);
}

std::vector<uint8_t> Extractor::getResource(const std::string& hash) const {
    auto it = this->resources.find(hash);
    if (it != this->resources.end())
        return it->second;
    throw std::runtime_error("Resource not found: " + hash);
}

std::vector<uint8_t> Pool::dxbcToSpirv(const std::vector<uint8_t>& dxbc) {
    // create sha1 hash of the dxbc data
    std::array<uint8_t, SHA_DIGEST_LENGTH> hash{};
    SHA1(dxbc.data(), dxbc.size(), hash.data());
    std::stringstream ss;
    for (const auto& byte : hash)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    const std::string hash_str = ss.str();

    // compile the shader
    dxvk::DxbcReader reader(reinterpret_cast<const char*>(dxbc.data()), dxbc.size());
    dxvk::DxbcModule module(reader);
    const dxvk::DxbcModuleInfo info{};
    auto shader = module.compile(info, "CS_" + hash_str);

    // extract spir-v from d3d11 shader
    auto code = shader->getRawCode();

    // patch binding offsets
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    for (size_t i = 0; i < shader->m_bindingOffsets.size(); i++)
        code.data()[shader->m_bindingOffsets.at(i).bindingOffset] = static_cast<uint8_t>(i); // NOLINT
#pragma clang diagnostic pop

    std::vector<uint8_t> spirv(code.size());
    std::copy_n(reinterpret_cast<uint8_t*>(code.data()),
        code.size(), spirv.data());
    return spirv;
}
