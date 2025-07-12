## lsfg-vk-v3.1
Version 3.1 of Lossless Scaling Frame Generation

This is a subproject of lsfg-vk and contains the external Vulkan logic for generating frames.

The project is intentionally structured as a fully external project, such that it can be integrated into other applications.

### Interface

Interfacing with lsfg-vk-v3.1 is done via `lsfg.hpp` header. The internal Vulkan instance is created using `LSFG_3_1::initialize()` and requires a specific deviceUUID, as well as parts of the lsfg-vk configuration, including a function loading SPIR-V shaders by name. Cleanup is done via `LSFG_3_1::finalize()` after which `LSFG_3_1::initialize()` may be called again. Please note that the initialization process is expensive and may take a while. It is recommended to call this function once during the applications lifetime.

Once the format and extent of the requested images is determined, `LSFG_3_1::createContext()` should be called to initialize a frame generation context. The Vulkan images are created from backing memory, which is passed through the file descriptor arguments. A context can be destroyed using `LSFG_3_1::deleteContext()`.

Presenting the context can be done via `LSFG_3_1::presentContext()`. Before calling the function a second time, make sure the outgoing semaphores have been signaled.
