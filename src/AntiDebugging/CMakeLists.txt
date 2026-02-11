add_library(AntiDebuggingPass MODULE 
    AntiDebugging.cpp
)

target_link_libraries(AntiDebuggingPass PRIVATE 
    ObfuscatorUtils
)
