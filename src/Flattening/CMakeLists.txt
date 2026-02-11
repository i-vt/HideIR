# Build the pass as a shared library (MODULE)
add_library(FlatteningPass MODULE 
    Flattening.cpp
)

# Link ONLY against our custom shared Utils. 
# Do NOT link against LLVM libraries here to avoid double-registration errors.
target_link_libraries(FlatteningPass PRIVATE 
    ObfuscatorUtils
)

