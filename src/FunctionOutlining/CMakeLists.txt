# Build the pass as a shared library (MODULE)
add_library(FunctionOutliningPass MODULE 
    FunctionOutlining.cpp
)

# Link ONLY against internal project dependencies.
target_link_libraries(FunctionOutliningPass PRIVATE 
    ObfuscatorUtils
)
