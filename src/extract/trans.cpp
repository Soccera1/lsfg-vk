#include "extract/trans.hpp"

#include <dxbc/dxbc_modinfo.h>
#include <dxbc/dxbc_module.h>
#include <dxbc/dxbc_reader.h>

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <vector>

using namespace Extract;

std::vector<uint8_t> Extract::translateShader(std::vector<uint8_t> bytecode) {
    // compile the shader
    dxvk::DxbcReader reader(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
    dxvk::DxbcModule module(reader);
    const dxvk::DxbcModuleInfo info{};
    auto shader = module.compile(info, "CS");

    // extract spir-v from d3d11 shader
    auto code = shader->getRawCode();

    // patch binding offsets
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    for (size_t i = 0; i < shader->m_bindingOffsets.size(); i++)
        code.data()[shader->m_bindingOffsets.at(i).bindingOffset] // NOLINT
            = static_cast<uint8_t>(i);
#pragma clang diagnostic pop

    // return the new bytecode
    std::vector<uint8_t> spirvBytecode(code.size());
    std::copy_n(reinterpret_cast<uint8_t*>(code.data()),
        code.size(), spirvBytecode.data());
    return spirvBytecode;
}
