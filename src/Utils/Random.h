#ifndef OBFUSCATOR_RANDOM_H
#define OBFUSCATOR_RANDOM_H

#include <cstdint>

namespace ObfuscatorUtils {
    class Random {
    public:
        // Generates a secure, uniformly distributed 32-bit unsigned integer
        static uint32_t generateRandomInt();

        // Generates a secure, uniformly distributed 32-bit integer within a specific range
        static uint32_t generateRandomIntInRange(uint32_t min, uint32_t max);
    };
} // namespace ObfuscatorUtils

#endif // OBFUSCATOR_RANDOM_H
