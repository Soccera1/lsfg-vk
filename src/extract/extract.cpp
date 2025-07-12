#include "extract/extract.hpp"

#include <pe-parse/parse.h>

#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace Extract;

const std::unordered_map<std::string, uint32_t> nameHashTable = {{
    { "mipmaps",  0xe365474d },
    { "alpha[0]", 0x0bf6c705 },
    { "alpha[1]", 0x4badbc24 },
    { "alpha[2]", 0x08a0f71c },
    { "alpha[3]", 0x5cc4d794 },
    { "beta[0]",  0x598efc9c },
    { "beta[1]",  0x17e25e8d },
    { "beta[2]",  0x6b8971f7 },
    { "beta[3]",  0x80a351c9 },
    { "beta[4]",  0xda0fcd5a },
    { "gamma[0]", 0x9dae0419 },
    { "gamma[1]", 0x96285646 },
    { "gamma[2]", 0xca44bb67 },
    { "gamma[3]", 0xa942fb59 },
    { "gamma[4]", 0x31996b8d },
    { "delta[0]", 0x9dae0419 },
    { "delta[1]", 0x67912aaa },
    { "delta[2]", 0xec138d48 },
    { "delta[3]", 0xdcb8247f },
    { "delta[4]", 0xd37c9bc8 },
    { "delta[5]", 0x62973dce },
    { "delta[6]", 0xe2ed5f66 },
    { "delta[7]", 0xc964a42a },
    { "delta[8]", 0x74536ad9 },
    { "delta[9]", 0xf472482a },
    { "generate", 0x5c040bd5 }
}};

namespace {
    auto& shaders() {
        static std::unordered_map<uint32_t, std::vector<uint8_t>> shaderData;
        return shaderData;
    }

    uint32_t fnv1a_hash(const std::vector<uint8_t>& data) {
        uint32_t hash = 0x811C9DC5;
        for (auto byte : data) {
            hash ^= byte;
            hash *= 0x01000193;
        }
        return hash;
    }

    int on_resource(void*, const peparse::resource& res) {
        if (res.type != peparse::RT_RCDATA || res.buf == nullptr || res.buf->bufLen <= 0)
            return 0;
        std::vector<uint8_t> resource_data(res.buf->bufLen);
        std::copy_n(res.buf->buf, res.buf->bufLen, resource_data.data());

        const uint32_t hash = fnv1a_hash(resource_data);
        shaders()[hash] = resource_data;
        return 0;
    }

    const std::vector<std::filesystem::path> PATHS{{
        ".local/share/Steam/steamapps/common",
        ".steam/steam/steamapps/common",
        ".steam/debian-installation/steamapps/common",
        ".var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common",
        "snap/steam/common/.local/share/Steam/steamapps/common"
    }};

    std::string getDllPath() {
        // overriden path
        const char* dllPath = getenv("LSFG_DLL_PATH");
        if (dllPath && *dllPath != '\0')
            return{dllPath};
        // home based paths
        const char* home = getenv("HOME");
        const std::string homeStr = home ? home : "";
        for (const auto& base : PATHS) {
            const std::filesystem::path path =
                std::filesystem::path(homeStr) / base / "Lossless Scaling" / "Lossless.dll";
            if (std::filesystem::exists(path))
                return path.string();
        }
        // xdg home
        const char* dataDir = getenv("XDG_DATA_HOME");
        if (dataDir && *dataDir != '\0')
            return std::string(dataDir) + "/Steam/steamapps/common/Lossless Scaling/Lossless.dll";
        // final fallback
        return "Lossless.dll";
    }
}

void Extract::extractShaders() {
    if (!shaders().empty())
        return;

    // parse the dll
    peparse::parsed_pe* dll = peparse::ParsePEFromFile(getDllPath().c_str());
    if (!dll)
        throw std::runtime_error("Unable to read Lossless.dll, is it installed?");
    peparse::IterRsrc(dll, on_resource, nullptr);
    peparse::DestructParsedPE(dll);
}

std::vector<uint8_t> Extract::getShader(const std::string& name) {
    if (shaders().empty())
        throw std::runtime_error("Shaders are not loaded.");

    auto hit = nameHashTable.find(name);
    if (hit == nameHashTable.end())
        throw std::runtime_error("Shader hash not found: " + name);

    auto sit = shaders().find(hit->second);
    if (sit == shaders().end())
        throw std::runtime_error("Shader not found: " + name);

    return sit->second;
}
