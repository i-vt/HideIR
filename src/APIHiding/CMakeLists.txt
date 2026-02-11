add_library(APIHidingPass MODULE 
    APIHiding.cpp
)

target_link_libraries(APIHidingPass PRIVATE 
    ObfuscatorUtils
)
