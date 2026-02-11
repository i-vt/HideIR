add_library(StringEncryptionPass MODULE 
    StringEncryption.cpp
)

# Link ONLY against internal project dependencies.
target_link_libraries(StringEncryptionPass PRIVATE 
    ObfuscatorUtils
)

# Removed visibility presets to ensure llvmGetPassPluginInfo is exported correctly.
