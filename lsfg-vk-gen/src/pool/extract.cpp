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

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace LSFG;
using namespace LSFG::Pool;

namespace {

    using ResourceMap = std::unordered_map<uint32_t, std::vector<uint8_t>>;

    uint32_t fnv1a_hash(const std::vector<uint8_t>& data) {
        // does not need be secure
        uint32_t hash = 0x811C9DC5;
        for (auto byte : data) {
            hash ^= byte;
            hash *= 0x01000193;
        }
        return hash;
    }

    /// Callback function for each resource.
    int on_resource(void* data, const peparse::resource& res) {
        if (res.type != peparse::RT_RCDATA || res.buf == nullptr || res.buf->bufLen <= 0)
            return 0;
        std::vector<uint8_t> resource_data(res.buf->bufLen);
        std::copy_n(res.buf->buf, res.buf->bufLen, resource_data.data());

        const uint32_t hash = fnv1a_hash(resource_data);

        auto* map = reinterpret_cast<ResourceMap*>(data);
        (*map)[hash] = resource_data;

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

std::vector<uint8_t> Extractor::getResource(uint32_t hash) const {
    auto it = this->resources.find(hash);
    if (it != this->resources.end())
        return it->second;
    throw std::runtime_error("Resource not found.");
}

std::vector<uint8_t> Pool::dxbcToSpirv(const std::vector<uint8_t>& dxbc) {
    // compile the shader
    dxvk::DxbcReader reader(reinterpret_cast<const char*>(dxbc.data()), dxbc.size());
    dxvk::DxbcModule module(reader);
    const dxvk::DxbcModuleInfo info{};
    auto shader = module.compile(info, "CS");

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
