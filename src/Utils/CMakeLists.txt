# Build the utility functions as a static library
add_library(ObfuscatorUtils STATIC
    Random.cpp
    Crypto.cpp
)

# This static library is linked into shared-object plugins (.so/.dylib),
# so it must be compiled with -fPIC.
set_target_properties(ObfuscatorUtils PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Allow other modules to easily include the headers from this directory
target_include_directories(ObfuscatorUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
