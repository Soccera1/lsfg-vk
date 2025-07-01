#ifndef LSFG_HPP
#define LSFG_HPP

#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/image.hpp"
#include "device.hpp"
#include "instance.hpp"
#include "shaderchains/alpha.hpp"
#include "shaderchains/beta.hpp"
#include "shaderchains/delta.hpp"
#include "shaderchains/downsample.hpp"
#include "shaderchains/epsilon.hpp"
#include "shaderchains/extract.hpp"
#include "shaderchains/gamma.hpp"
#include "shaderchains/magic.hpp"
#include "shaderchains/merge.hpp"
#include "shaderchains/zeta.hpp"
#include "utils.hpp"

namespace LSFG {

    class Generator;

    /// LSFG context.
    class Context {
        friend class Generator; // FIXME: getters, I'm lazy
    public:
        ///
        /// Initialize the LSFG Vulkan instance.
        ///
        /// @throws LSFG::vulkan_error if the Vulkan objects cannot be created.
        ///
        Context() { Globals::initializeGlobals(device); } // FIXME: no need for globals

        ///
        /// Create a generator instance.
        ///
        /// @throws LSFG::vulkan_error if the generator cannot be created.
        ///
        const Generator& create();

        ///
        /// Present a generator instance.
        ///
        /// @throws LSFG::vulkan_error if the generator fails to present.
        ///
        void present(const Generator& gen);

        /// Trivial copyable, moveable and destructible
        Context(const Context&) = default;
        Context& operator=(const Context&) = default;
        Context(Context&&) = default;
        Context& operator=(Context&&) = default;
        ~Context() { Globals::uninitializeGlobals(); }
    private:
        Instance instance;
        Device device{instance};
        Core::DescriptorPool descPool{device};
        Core::CommandPool cmdPool{device};
    };

    /// Per-swapchain instance of LSFG.
    class Generator {
    public:
        ///
        /// Create a generator instance.
        ///
        /// @param context The LSFG context to use.
        ///
        Generator(const Context& context);

        ///
        /// Present.
        ///
        /// @throws LSFG::vulkan_error if the generator fails to present.
        ///
        void present(const Context& context);

        // Trivially copyable, moveable and destructible
        Generator(const Generator&) = default;
        Generator(Generator&&) = default;
        Generator& operator=(const Generator&) = default;
        Generator& operator=(Generator&&) = default;
        ~Generator() = default;
    private:
        Core::Image inImg_0, inImg_1; // inImg_0 is next (inImg_1 prev) when fc % 2 == 0
        uint64_t fc{0};

        Shaderchains::Downsample downsampleChain; // FIXME: get rid of default constructors (+ core)
        std::array<Shaderchains::Alpha, 7> alphaChains;
        Shaderchains::Beta betaChain;
        std::array<Shaderchains::Gamma, 4> gammaChains;
        std::array<Shaderchains::Magic, 3> magicChains;
        std::array<Shaderchains::Delta, 3> deltaChains;
        std::array<Shaderchains::Epsilon, 3> epsilonChains;
        std::array<Shaderchains::Zeta, 3> zetaChains;
        std::array<Shaderchains::Extract, 2> extractChains;
        Shaderchains::Merge mergeChain;
    };

    /// Simple exception class for Vulkan errors.
    class vulkan_error : public std::runtime_error {
    public:
        ///
        /// Construct a vulkan_error with a message and a Vulkan result code.
        ///
        /// @param result The Vulkan result code associated with the error.
        /// @param message The error message.
        ///
        explicit vulkan_error(VkResult result, const std::string& message);

        /// Get the Vulkan result code associated with this error.
        [[nodiscard]] VkResult error() const { return this->result; }

        // Trivially copyable, moveable and destructible
        vulkan_error(const vulkan_error&) = default;
        vulkan_error(vulkan_error&&) = default;
        vulkan_error& operator=(const vulkan_error&) = default;
        vulkan_error& operator=(vulkan_error&&) = default;
        ~vulkan_error() noexcept override;
    private:
        VkResult result;
    };

}

#endif // LSFG_HPP
