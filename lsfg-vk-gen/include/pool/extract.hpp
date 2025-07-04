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
    /// DirectX bytecode translator class.
    ///
    class Translator {
    public:
        ///
        /// Create a new translator.
        ///
        /// @throws std::runtime_error if the initialization fails.
        ///
        Translator();

        ///
        /// Translate DXBC into SPIR-V.
        ///
        /// @param dxbc Bytecode to translate.
        /// @return Translated SPIR-V bytecode.
        ///
        /// @throws std::runtime_error if the translation fails.
        ///
        [[nodiscard]] std::vector<uint8_t> translate(const std::vector<uint8_t>& dxbc) const;

        // Trivially copyable, moveable and destructible
        Translator(const Translator&) = delete;
        Translator& operator=(const Translator&) = delete;
        Translator(Translator&&) = default;
        Translator& operator=(Translator&&) = default;
        ~Translator() = default;
    private:
        std::shared_ptr<ID3D11Device*> device;
        std::shared_ptr<ID3D11DeviceContext*> context;
    };

}


#endif // RESOURCES_HPP
