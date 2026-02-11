# tests/CMakeLists.txt

# Locate the LLVM binaries
set(LLVM_TOOLS_BINARY_DIR "${LLVM_BINARY_DIR}/bin")

# Process the site config template
configure_file(
  lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  @ONLY
)

# Define the test suite and its dependencies
add_lit_testsuite(check-obfuscator "Running Enterprise Obfuscator Tests"
  ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS 
    SplitBasicBlockPass
    FlatteningPass
    OpaquePredicatePass
    StringEncryptionPass
    FunctionOutliningPass
  PARAMS
    obfuscator_site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
)
