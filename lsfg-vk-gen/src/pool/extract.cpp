#include "pool/extract.hpp"

#include <d3d11/d3d11_device.h>
#include <d3d11.h>
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

Translator::Translator() {
    setenv("DXVK_WSI_DRIVER", "SDL3", 0);
    setenv("DXVK_LOG_LEVEL", "error", 0);
    setenv("DXVK_LOG_PATH", "none", 0);

    // create d3d11 device
    ID3D11Device* device{};
    ID3D11DeviceContext* context{};
    const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
    D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        &featureLevel, 1,
        D3D11_SDK_VERSION,
        &device, nullptr, &context
    );
    if (!device || !context)
        throw std::runtime_error("Failed to create D3D11 device");

    // store device in shared ptr
    this->device = std::shared_ptr<ID3D11Device*>(
        new ID3D11Device*(device),
        [](ID3D11Device** dev) {
            (*dev)->Release();
        }
    );
    this->context = std::shared_ptr<ID3D11DeviceContext*>(
        new ID3D11DeviceContext*(context),
        [](ID3D11DeviceContext** ctx) {
            (*ctx)->Release();
        }
    );
}

std::vector<uint8_t> Translator::translate(const std::vector<uint8_t>& dxbc) const {
    // create compute shader and pipeline
    ID3D11ComputeShader* shader = nullptr;
    (*this->device)->CreateComputeShader(dxbc.data(), dxbc.size(), nullptr, &shader);
    if (!shader)
        throw std::runtime_error("Failed to create compute shader from DXBC");

    auto* dxvk_shader = reinterpret_cast<dxvk::D3D11ComputeShader*>(shader);
    auto* dxvk_device = reinterpret_cast<dxvk::D3D11Device*>(*this->device);

    auto* pipeline = dxvk_device->GetDXVKDevice()->m_objects.pipelineManager().createComputePipeline({
        .cs = dxvk_shader->GetCommonShader()->GetShader()
    });

    // extract spir-v from d3d11 shader
    auto code = dxvk_shader->GetCommonShader()->GetShader()->getCode(
        pipeline->getBindings(), dxvk::DxvkShaderModuleCreateInfo());
    std::vector<uint8_t> spirv(code.size());
    std::copy_n(reinterpret_cast<uint8_t*>(code.data()),
        code.size(), spirv.data());

    // cleanup-ish (i think the pipeline will linger)
    shader->Release();
    return spirv;
}
