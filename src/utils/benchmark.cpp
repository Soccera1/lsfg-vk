#include "extract/extract.hpp"
#include "extract/trans.hpp"
#include "utils/log.hpp"

#include <vulkan/vulkan_core.h>
#include <lsfg.hpp>

#include <cstdint>
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
    void __attribute__((constructor)) init() {
        // continue if preloaded
        const char* preload = std::getenv("LD_PRELOAD");
        if (!preload || *preload == '\0')
            return;
        const std::string preload_str(preload);
        if (preload_str.find("liblsfg-vk.so") == std::string::npos)
            return;
        // continue if benchmark requested
        const char* benchmark = std::getenv("LSFG_BENCHMARK");
        if (!benchmark || *benchmark == '\0')
            return;
        const std::string benchmark_str(benchmark);
        if (benchmark_str != "1")
            return;

        // fetch benchmark parameters
        const char* lsfgFlowScale = std::getenv("LSFG_FLOW_SCALE");
        const char* lsfgHdr = std::getenv("LSFG_HDR");
        const char* lsfgMultiplier = std::getenv("LSFG_MULTIPLIER");
        const char* lsfgExtentWidth = std::getenv("LSFG_EXTENT_WIDTH");
        const char* lsfgExtentHeight = std::getenv("LSFG_EXTENT_HEIGHT");

        const float flowScale = lsfgFlowScale
            ? std::stof(lsfgFlowScale) : 1.0F;
        const bool isHdr = lsfgHdr
            ? *lsfgHdr == '1' : false;
        const uint64_t multiplier = lsfgMultiplier
            ? std::stoull(std::string(lsfgMultiplier)) : 2;
        const uint32_t width = lsfgExtentWidth
            ? static_cast<uint32_t>(std::stoul(lsfgExtentWidth)) : 1920;
        const uint32_t height = lsfgExtentHeight
            ? static_cast<uint32_t>(std::stoul(lsfgExtentHeight)) : 1080;

        Log::info("bench", "Running {}x benchmark with {}x{} extent and flow scale of {} {} HDR",
            multiplier, width, height, flowScale, isHdr ? "with" : "without");

        // create the benchmark context
        const char* lsfgDeviceUUID = std::getenv("LSFG_DEVICE_UUID");
        const uint64_t deviceUUID = lsfgDeviceUUID
            ? std::stoull(std::string(lsfgDeviceUUID), nullptr, 16) : 0x1463ABAC;

        Extract::extractShaders();
        LSFG::initialize(
            deviceUUID, // some magic number if not given
            isHdr, 1.0F / flowScale, multiplier - 1,
            [](const std::string& name) -> std::vector<uint8_t> {
                auto dxbc = Extract::getShader(name);
                auto spirv = Extract::translateShader(dxbc);
                return spirv;
            }
        );
        const int32_t ctx = LSFG::createContext(-1, -1, {},
            { .width = width, .height = height },
            isHdr ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM
        );

        Log::info("bench", "Benchmark context created, ready to run");

        // run the benchmark (run 8*n + 1 so the fences are waited on)
        const auto now = std::chrono::high_resolution_clock::now();
        const uint64_t iterations = (8 * 500) + 1;
        for (uint64_t count = 0; count < iterations; count++)
            LSFG::presentContext(ctx, -1, {});
        const auto then = std::chrono::high_resolution_clock::now();

        // print results
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(then - now).count();

        const auto perIteration = static_cast<float>(ms) / static_cast<float>(iterations);

        const uint64_t totalGen = (multiplier - 1) * iterations;
        const auto genFps = static_cast<float>(totalGen) / (static_cast<float>(ms) / 1000.0F);

        const uint64_t totalFrames = iterations * multiplier;
        const auto totalFps = static_cast<float>(totalFrames) / (static_cast<float>(ms) / 1000.0F);

        Log::info("bench", "Benchmark completed in {} ms", ms);
        Log::info("bench", "Time per iteration: {:.2f} ms", perIteration);
        Log::info("bench", "Generation FPS: {:.2f}", genFps);
        Log::info("bench", "Final FPS: {:.2f}", totalFps);
        Log::info("bench", "Benchmark finished, exiting");
    }
}
