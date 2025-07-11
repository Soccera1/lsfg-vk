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
    { "alpha[0]", 0xf37c8aa8 },
    { "alpha[1]", 0xeb7a52d4 },
    { "alpha[2]", 0x8901788e },
    { "alpha[3]", 0xa06a5e36 },
    { "beta[0]",  0x63c16b89 },
    { "beta[1]",  0xe3967ed5 },
    { "beta[2]",  0x570085ee },
    { "beta[3]",  0x4f4530db },
    { "beta[4]",  0x39727389 },
    { "gamma[0]", 0x94c4edf6 },
    { "gamma[1]", 0xf4e32702 },
    { "gamma[2]", 0xa3dc56fc },
    { "gamma[3]", 0x8b5ed8f6 },
    { "gamma[4]", 0x1cbf3c4d },
    { "delta[0]", 0x94c4edf6 },
    { "delta[1]", 0xacfd805b },
    { "delta[2]", 0x891dc48b },
    { "delta[3]", 0x98536d9d },
    { "delta[4]", 0x8e3f2155 },
    { "delta[5]", 0x8f0e70a1 },
    { "delta[6]", 0xd5eca8f1 },
    { "delta[7]", 0xa9e54e37 },
    { "delta[8]", 0x1dee8b84 },
    { "delta[9]", 0x1576c3f5 },
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
