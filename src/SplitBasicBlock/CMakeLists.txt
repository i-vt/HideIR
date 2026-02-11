add_library(SplitBasicBlockPass MODULE 
    SplitBasicBlock.cpp
)

# Link ONLY against internal project dependencies.
target_link_libraries(SplitBasicBlockPass PRIVATE 
    ObfuscatorUtils
)

# Removed visibility presets to ensure llvmGetPassPluginInfo is exported correctly.
