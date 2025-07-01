#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "log.hpp"

extern "C" void __attribute__((constructor)) init();
extern "C" [[noreturn]] void __attribute__((destructor)) deinit();

void init() {
    Log::info("lsfg-vk: init() called");

    // hook loaders
    Loader::DL::initialize();
    Loader::VK::initialize();
}

void deinit() {
    Log::debug("lsfg-vk: deinit() called, exiting");
    // for some reason some applications unload the library despite it containing
    // the dl functions. this will lead to a segmentation fault, so we exit early.
    exit(EXIT_SUCCESS);
}
