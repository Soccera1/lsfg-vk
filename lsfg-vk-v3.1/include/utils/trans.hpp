#ifndef TRANS_HPP
#define TRANS_HPP

#include <cstdint>
#include <vector>

namespace LSFG::Utils::Trans {

    ///
    /// Translate shader bytecode to SPIR-V.
    ///
    /// @param bytecode The shader bytecode to translate.
    /// @return A vector containing the translated SPIR-V bytecode.
    ///
    [[nodiscard]] std::vector<uint8_t> translateShader(std::vector<uint8_t> bytecode);

}

#endif // TRANS_HPP
