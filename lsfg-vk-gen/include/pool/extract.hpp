#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include <dxbc/dxbc_options.h>
#include <d3d11.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace LSFG::Pool {

    ///
    /// DLL resource extractor class.
    ///
    class Extractor {
    public:
        Extractor() noexcept = default;

        ///
        /// Create a new extractor.
        ///
        /// @param path Path to the DLL file.
        ///
        /// @throws std::runtime_error if the file cannot be parsed.
        ///
        Extractor(const std::string& path);

        ///
        /// Get a resource by its hash.
        ///
        /// @param hash Hash of the resource.
        /// @return Resource data
        ///
        /// @throws std::runtime_error if the resource is not found.
        ///
        [[nodiscard]] std::vector<uint8_t> getResource(const std::string& hash) const;

        // Trivially copyable, moveable and destructible
        Extractor(const Extractor&) = delete;
        Extractor& operator=(const Extractor&) = delete;
        Extractor(Extractor&&) = default;
        Extractor& operator=(Extractor&&) = default;
        ~Extractor() = default;
    private:
        std::unordered_map<std::string, std::vector<uint8_t>> resources;
    };

    ///
    /// Translate DXBC into SPIR-V.
    ///
    /// @param dxbc Bytecode to translate.
    /// @return Translated SPIR-V bytecode.
    ///
    /// @throws std::runtime_error if the translation fails.
    ///
    [[nodiscard]] std::vector<uint8_t> dxbcToSpirv(const std::vector<uint8_t>& dxbc);

}


#endif // RESOURCES_HPP
