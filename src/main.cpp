#include "lsfg.hpp"

#include <iostream>

#include <renderdoc_app.h>
#include <dlfcn.h>
#include <unistd.h>

using namespace LSFG;

int main() {
    // attempt to load renderdoc
    RENDERDOC_API_1_6_0* rdoc = nullptr;
    if (void* mod_renderdoc = dlopen("/usr/lib/librenderdoc.so", RTLD_NOLOAD | RTLD_NOW)) {
        std::cerr << "Found RenderDoc library, setting up frame capture." << '\n';

        auto GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod_renderdoc, "RENDERDOC_GetAPI"));
        const int ret = GetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&rdoc));
        if (ret == 0) {
            std::cerr << "Unable to initialize RenderDoc API. Is your RenderDoc up to date?" << '\n';
            rdoc = nullptr;
        }
        usleep(1000 * 100); // give renderdoc time to load
    }

    // initialize test application
    LSFG::Context context;
    auto gen = LSFG::Generator(context);

    for (int i = 0; i < 3; i++) {
        if (rdoc)
            rdoc->StartFrameCapture(nullptr, nullptr);

        gen.present(context);

        if (rdoc)
            rdoc->EndFrameCapture(nullptr, nullptr);

        // sleep 8 ms
        usleep(8000);
    }

    usleep(1000 * 100);

    std::cerr << "Application finished" << '\n';
    return 0;
}
