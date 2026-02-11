add_subdirectory(Utils)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

add_subdirectory(Flattening)
add_subdirectory(OpaquePredicate)
add_subdirectory(SplitBasicBlock)
add_subdirectory(StringEncryption)
add_subdirectory(FunctionOutlining)
add_subdirectory(AntiDebugging)
add_subdirectory(APIHiding)
add_subdirectory(AntiTampering)
