#ifndef OBFUSCATOR_CRYPTO_H
#define OBFUSCATOR_CRYPTO_H

#include <string>
#include <vector>
#include <cstdint>

namespace ObfuscatorUtils {
    class Crypto {
    public:
        // Perform a simple XOR encryption on a plaintext string
        static std::vector<uint8_t> xorEncrypt(const std::string& plaintext, uint8_t key);

        // Decrypt an XOR-encrypted byte array back to a string
        static std::string xorDecrypt(const std::vector<uint8_t>& ciphertext, uint8_t key);
    };
} // namespace ObfuscatorUtils

#endif // OBFUSCATOR_CRYPTO_H
