#include "Random.h"
#include <random>

namespace ObfuscatorUtils {

    uint32_t Random::generateRandomInt() {
        // Use random_device to seed a Mersenne Twister engine
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;
        
        return dis(gen);
    }

    uint32_t Random::generateRandomIntInRange(uint32_t min, uint32_t max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(min, max);
        
        return dis(gen);
    }

} // namespace ObfuscatorUtils
