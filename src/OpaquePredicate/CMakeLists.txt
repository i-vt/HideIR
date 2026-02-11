add_library(OpaquePredicatePass MODULE 
    OpaquePredicate.cpp
)

# Link ONLY against internal project dependencies.
target_link_libraries(OpaquePredicatePass PRIVATE 
    ObfuscatorUtils
)

