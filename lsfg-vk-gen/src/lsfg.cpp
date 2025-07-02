#include "lsfg.hpp"
#include "core/device.hpp"
#include "core/instance.hpp"
#include "context.hpp"
#include "utils.hpp"

#include <ctime>
#include <optional>
#include <unordered_map>

using namespace LSFG;

namespace {
    std::optional<Core::Instance> instance;
    std::optional<Core::Device> device;
    std::unordered_map<int32_t, Context> contexts;
}

void LSFG::initialize() {
    if (instance.has_value() || device.has_value())
        return;

    instance.emplace();
    device.emplace(*instance);

    Globals::initializeGlobals(*device);

    std::srand(static_cast<uint32_t>(std::time(nullptr)));
}

int32_t LSFG::createContext(uint32_t width, uint32_t height, int in0, int in1,
        const std::vector<int>& outN) {
    if (!instance.has_value() || !device.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto id = std::rand();
    contexts.emplace(id, Context(*device, width, height, in0, in1, outN));
    return id;
}

void LSFG::presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    if (!instance.has_value() || !device.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto it = contexts.find(id);
    if (it == contexts.end())
        throw LSFG::vulkan_error(VK_ERROR_DEVICE_LOST, "No such context");

    Context& context = it->second;
    context.present(*device, inSem, outSem);
}

void LSFG::deleteContext(int32_t id) {
    if (!instance.has_value() || !device.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "LSFG not initialized");

    auto it = contexts.find(id);
    if (it == contexts.end())
        throw LSFG::vulkan_error(VK_ERROR_DEVICE_LOST, "No such context");

    contexts.erase(it);
}

void LSFG::finalize() {
    if (!instance.has_value() && !device.has_value())
        return;

    Globals::uninitializeGlobals();

    instance.reset();
    device.reset();
}
