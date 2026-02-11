#include "Crypto.h"

namespace ObfuscatorUtils {

    std::vector<uint8_t> Crypto::xorEncrypt(const std::string& plaintext, uint8_t key) {
        std::vector<uint8_t> ciphertext;
        ciphertext.reserve(plaintext.size());
        
        for (char c : plaintext) {
            // XOR each character with the provided key
            ciphertext.push_back(static_cast<uint8_t>(c) ^ key);
        }
        
        return ciphertext;
    }

    std::string Crypto::xorDecrypt(const std::vector<uint8_t>& ciphertext, uint8_t key) {
        std::string plaintext;
        plaintext.reserve(ciphertext.size());
        
        for (uint8_t c : ciphertext) {
            // XOR back to decrypt
            plaintext.push_back(static_cast<char>(c ^ key));
        }
        
        return plaintext;
    }

} // namespace ObfuscatorUtils
