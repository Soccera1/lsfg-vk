#include "utils/exceptions.hpp"

#include <format>

using namespace ls;

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(std::format("{} (error {})", message, static_cast<uint32_t>(result))), result(result) {}

vulkan_error::~vulkan_error() noexcept = default;
