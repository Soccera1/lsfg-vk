#include "utils/trans.hpp"

#include <vector>
#include <cstdint>

using namespace LSFG::Utils;

std::vector<uint8_t> Trans::translateShader(std::vector<uint8_t> bytecode) {
    return bytecode; // on windows we expect the bytecode to be spir-v
//     // compile the shader
//     dxvk::DxbcReader reader(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());
//     dxvk::DxbcModule module(reader);
//     const dxvk::DxbcModuleInfo info{};
//     auto shader = module.compile(info, "CS");

//     // extract spir-v from d3d11 shader
//     auto code = shader->getRawCode();

//     // patch binding offsets
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
//     for (size_t i = 0; i < shader->m_bindingOffsets.size(); i++)
//         code.data()[shader->m_bindingOffsets.at(i).bindingOffset] = static_cast<uint8_t>(i); // NOLINT
// #pragma clang diagnostic pop

//     std::vector<uint8_t> spirvBytecode(code.size());
//     std::copy_n(reinterpret_cast<uint8_t*>(code.data()),
//         code.size(), spirvBytecode.data());
//     return spirvBytecode;
// #endif
}
