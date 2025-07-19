#include "config/config.hpp"
#include "common/exception.hpp"

#include "config/default_conf.hpp"

#include <vulkan/vulkan_core.h>
#include <toml11/find.hpp>
#include <toml11/parser.hpp>
#include <toml.hpp>

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>
#include <string>

using namespace Config;

namespace {
    Configuration globalConf{};
    std::optional<std::unordered_map<std::string, Configuration>> gameConfs;
}

Configuration Config::activeConf{};

namespace {
    /// Turn a string into a VkPresentModeKHR enum value.
    VkPresentModeKHR into_present(const std::string& mode) {
        if (mode == "fifo" || mode == "vsync")
            return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        if (mode == "mailbox")
            return VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
        if (mode == "immediate")
            return VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
        return VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    }

    /// Parse environment variables from a string.
    std::vector<std::pair<std::string, std::string>> parse_env(const std::string& envs) {
        std::vector<std::pair<std::string, std::string>> vars{};
        const std::string env_str = envs + ' ';

        std::string current{};
        bool escape{false};
        for (const char c : env_str) {
            // toggle escape mode
            if (c == '\'') {
                escape = !escape;
                continue;
            }

            // parse variable
            if (c == ' ' && !escape) {
                if (current.empty())
                    continue;

                auto eq_pos = current.find('=');
                if (eq_pos == std::string::npos)
                    throw std::runtime_error("Invalid environment variable: " + current);

                std::string key = current.substr(0, eq_pos);
                std::string value = current.substr(eq_pos + 1);
                if (key.empty() || value.empty())
                    throw std::runtime_error("Invalid environment variable: " + current);

                vars.emplace_back(std::move(key), std::move(value));

                current.clear();
                continue;
            }

            current += c;
        }

        return vars;
    }
}

void Config::updateConfig(const std::string& file) {
    if (!std::filesystem::exists(file)) {
        std::cerr << "lsfg-vk: Placing default configuration file at " << file << '\n';
        const auto parent = std::filesystem::path(file).parent_path();
        if (!std::filesystem::exists(parent))
            if (!std::filesystem::create_directories(parent))
                throw std::runtime_error("Unable to create configuration directory at " + parent.string());

        std::ofstream out(file);
        if (!out.is_open())
            throw std::runtime_error("Unable to create configuration file at " + file);
        out << DEFAULT_CONFIG;
        out.close();
    }

    // parse config file
    std::optional<toml::value> parsed;
    try {
        parsed.emplace(toml::parse(file));
        if (!parsed->contains("version"))
            throw std::runtime_error("Configuration file is missing 'version' field");
        if (parsed->at("version").as_integer() != 1)
            throw std::runtime_error("Configuration file version is not supported, expected 1");
    } catch (const std::exception& e) {
        throw LSFG::rethrowable_error("Unable to parse configuration file", e);
    }
    auto& toml = *parsed;

    // parse global configuration
    const toml::value globalTable = toml::find_or_default<toml::table>(toml, "global");
    const Configuration global{
        .dll =   toml::find_or(globalTable, "dll", std::string()),
        .config_file = file,
        .timestamp = std::filesystem::last_write_time(file)
    };

    // validate global configuration
    if (global.multiplier < 2)
        throw std::runtime_error("Global Multiplier cannot be less than 2");
    if (global.flowScale < 0.25F || global.flowScale > 1.0F)
        throw std::runtime_error("Flow scale must be between 0.25 and 1.0");

    // parse game-specific configuration
    std::unordered_map<std::string, Configuration> games;
    const toml::value gamesList = toml::find_or_default<toml::array>(toml, "game");
    for (const auto& gameTable : gamesList.as_array()) {
        if (!gameTable.is_table())
            throw std::runtime_error("Invalid game configuration entry");
        if (!gameTable.contains("exe"))
            throw std::runtime_error("Game override missing 'exe' field");

        const std::string exe = toml::find<std::string>(gameTable, "exe");
        Configuration game{
            .enable = true,
            .dll = global.dll,
            .env = parse_env(toml::find_or(gameTable, "env", std::string())),
            .multiplier = toml::find_or(gameTable, "multiplier", 2U),
            .flowScale = toml::find_or(gameTable, "flow_scale", 1.0F),
            .performance = toml::find_or(gameTable, "performance_mode", false),
            .hdr = toml::find_or(gameTable, "hdr_mode", false),
            .e_present =   into_present(toml::find_or(gameTable, "experimental_present_mode", "")),
            .e_fps_limit = toml::find_or(gameTable, "experimental_fps_limit", 0U),
            .config_file = file,
            .timestamp = global.timestamp
        };

        // validate the configuration
        if (game.multiplier < 1)
            throw std::runtime_error("Multiplier cannot be less than 1");
        if (game.flowScale < 0.25F || game.flowScale > 1.0F)
            throw std::runtime_error("Flow scale must be between 0.25 and 1.0");
        games[exe] = std::move(game);
    }

    // store configurations
    globalConf = global;
    gameConfs = std::move(games);
}

Configuration Config::getConfig(const std::pair<std::string, std::string>& name) {
    // process legacy environment variables
    if (std::getenv("LSFG_LEGACY")) {
        Configuration conf{
            .enable = true,
            .multiplier = 2,
            .flowScale = 1.0F,
            .e_present = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR
        };

        const char* dll = std::getenv("LSFG_DLL_PATH");
        if (dll) conf.dll = std::string(dll);
        const char* multiplier = std::getenv("LSFG_MULTIPLIER");
        if (multiplier) conf.multiplier = std::stoul(multiplier);
        const char* flow_scale = std::getenv("LSFG_FLOW_SCALE");
        if (flow_scale) conf.flowScale = std::stof(flow_scale);
        const char* performance = std::getenv("LSFG_PERFORMANCE_MODE");
        if (performance) conf.performance = std::string(performance) == "1";
        const char* hdr = std::getenv("LSFG_HDR_MODE");
        if (hdr) conf.hdr = std::string(hdr) == "1";
        const char* e_present = std::getenv("LSFG_EXPERIMENTAL_PRESENT_MODE");
        if (e_present) conf.e_present = into_present(std::string(e_present));
        const char* e_fps_limit = std::getenv("LSFG_EXPERIMENTAL_FPS_LIMIT");
        if (e_fps_limit) conf.e_fps_limit = static_cast<uint32_t>(std::stoul(e_fps_limit));
        const char* envs = std::getenv("LSFG_ENV");
        if (envs) conf.env = parse_env(std::string(envs));

        return conf;
    }

    // process new configuration system
    if (!gameConfs.has_value())
        return globalConf;

    const auto& games = *gameConfs;
    auto it = std::ranges::find_if(games, [&name](const auto& pair) {
        return name.first.ends_with(pair.first) || (name.second == pair.first);
    });
    if (it != games.end())
        return it->second;

    return globalConf;
}
