#include "device.hpp"
#include "instance.hpp"

#include <iostream>

int main() {
    const Vulkan::Instance instance;
    const Vulkan::Device device(instance);

    std::cerr << "Application finished" << '\n';
    return 0;
}
