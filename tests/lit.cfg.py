import os
import lit.formats
from lit.llvm import llvm_config

config.name = 'EnterpriseObfuscator'
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)
config.suffixes = ['.ll']
config.test_source_root = os.path.dirname(__file__)

# Map the paths from the site config to lit substitutions
# Braced syntax %{name} is required to prevent %s from mangling the paths
config.substitutions.append(('%{split_plugin}', config.split_plugin_path))
config.substitutions.append(('%{opaque_plugin}', config.opaque_plugin_path))
config.substitutions.append(('%{flattening_plugin}', config.flattening_plugin_path))
config.substitutions.append(('%{string_plugin}', config.string_plugin_path))
config.substitutions.append(('%{outlining_plugin}', config.outlining_plugin_path))

# Add LLVM tools (opt, FileCheck) to the PATH for the tests
llvm_config.add_tool_substitutions(['opt', 'FileCheck'], config.llvm_tools_dir)
