#ifndef LOG_HPP
#define LOG_HPP

#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>

namespace Log {

    namespace Internal {
        extern bool isSetup;

        extern std::set<std::string> debugModules;
        extern bool debugAllModules;

        extern std::ofstream logFile;
        extern std::mutex logMutex;

        void setup();
    }

    template<typename... Args>
    void log(std::string_view color, std::string_view module,
            std::format_string<Args...> fmt, Args&&... args) {
        Internal::setup();

        std::string prefix = std::format("lsfg-vk({}): ", module);
        std::string message = std::format(fmt, std::forward<Args>(args)...);

        std::lock_guard<std::mutex> lock(Internal::logMutex);
        std::cerr << color << prefix << message << "\033[0m" << '\n';
        if (Internal::logFile.is_open()) {
            Internal::logFile << prefix << message << '\n';
            Internal::logFile.flush();
        }
    }

    const std::string_view WHITE = "\033[1;37m";
    const std::string_view YELLOW = "\033[1;33m";
    const std::string_view RED = "\033[1;31m";

    template<typename... Args>
    void info(std::string_view module, std::format_string<Args...> fmt, Args&&... args) {
        log(WHITE, module, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(std::string_view module, std::format_string<Args...> fmt, Args&&... args) {
        log(YELLOW, module, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(std::string_view module, std::format_string<Args...> fmt, Args&&... args) {
        log(RED, module, fmt, std::forward<Args>(args)...);
    }

    const std::string_view GRAY = "\033[1;90m";

#ifdef LSFG_NO_DEBUG
template<typename... Args>
void debug(std::string_view, std::format_string<Args...>, Args&&...) {} // NOLINT
#else
    template<typename... Args>
    void debug(std::string_view module, std::format_string<Args...> fmt, Args&&... args) {
        Internal::setup();
        if (Internal::debugAllModules || Internal::debugModules.contains(std::string(module)))
            log(GRAY, module, fmt, std::forward<Args>(args)...);
    }
#endif

}

#endif // LOG_HPP
