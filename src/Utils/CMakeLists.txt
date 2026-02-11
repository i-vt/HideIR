# Build the utility functions as a static library
add_library(ObfuscatorUtils STATIC
    Random.cpp
    Crypto.cpp
)

# Allow other modules to easily include the headers from this directory
target_include_directories(ObfuscatorUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
