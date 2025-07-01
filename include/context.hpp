#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "core/commandpool.hpp"
#include "core/descriptorpool.hpp"
#include "core/image.hpp"
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

namespace LSFG {

    ///
    /// LSFG context.
    ///
    class Context {
    public:
        ///
        /// Create a generator instance.
        ///
        /// @param device The Vulkan device to use.
        /// @param width Width of the input images.
        /// @param height Height of the input images.
        /// @param in0 File descriptor for the first input image.
        /// @param in1 File descriptor for the second input image.
        ///
        /// @throws LSFG::vulkan_error if the generator fails to initialize.
        ///
        Context(const Core::Device& device, uint32_t width, uint32_t height, int in0, int in1);

        ///
        /// Schedule the next generation.
        ///
        /// @param device The Vulkan device to use.
        /// @param inSem Semaphore to wait on before starting the generation.
        /// @param outSem Semaphore to signal when the generation is complete.
        ///
        /// @throws LSFG::vulkan_error if the generator fails to present.
        ///
        void present(const Core::Device& device, int inSem, int outSem);

        // Trivially copyable, moveable and destructible
        Context(const Context&) = default;
        Context(Context&&) = default;
        Context& operator=(const Context&) = default;
        Context& operator=(Context&&) = default;
        ~Context() = default;
    private:
        Core::DescriptorPool descPool;
        Core::CommandPool cmdPool;

        Core::Image inImg_0, inImg_1; // inImg_0 is next (inImg_1 prev) when fc % 2 == 0
        uint64_t fc{0};

        Shaderchains::Downsample downsampleChain;
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

}

#endif // CONTEXT_HPP
