add_library(AntiTamperingPass MODULE 
    AntiTampering.cpp
)

target_link_libraries(AntiTamperingPass PRIVATE 
    ObfuscatorUtils
)
