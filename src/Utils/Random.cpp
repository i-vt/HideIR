#include "Random.h"
#include <random>

namespace ObfuscatorUtils {

    // Static engine seeded once, avoiding repeated std::random_device +
    // std::mt19937 construction on every call. LLVM passes run single-threaded
    // per compilation unit, so thread_local is unnecessary (and produces TLS
    // relocations that break when linked into shared-object plugins).
    static std::mt19937 &getEngine() {
        static std::mt19937 engine{std::random_device{}()};
        return engine;
    }

    uint32_t Random::generateRandomInt() {
        std::uniform_int_distribution<uint32_t> dis;
        return dis(getEngine());
    }

    uint32_t Random::generateRandomIntInRange(uint32_t min, uint32_t max) {
        std::uniform_int_distribution<uint32_t> dis(min, max);
        return dis(getEngine());
    }

} // namespace ObfuscatorUtils
