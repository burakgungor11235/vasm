vm_sources = [
  'vm/core.c',
  'vm/registers.c',
  'vm/memory.c',
  'vm/condition.c',
  'vm/instruction.c',
]

vm_headers = [
  '../../include/vm.h',
  '../../include/opcode.h',
]

asm_sources = [
  'asm/lexer.c',
  'asm/parser.c',
  'asm/directives.c',
  'asm/symbol.c',
]

debug_sources = [
  'debug/debugger.c',
  'debug/cmd.c',
  'debug/disasm.c',
]

executable('varm',
  'main.c',
  vm_sources,
  asm_sources,
  debug_sources,
  include_directories: include_directories('../../include'),
  dependencies: [],
)

executable('vasm',
  'main_asm.c',
  asm_sources,
  include_directories: include_directories('../../include'),
  dependencies: [],
)

# Unit test executables
unit_lexer = executable('unit_lexer', 'test/unit/lexer.c',
  include_directories: include_directories('../../include'),
)

unit_parser = executable('unit_parser', 'test/unit/parser.c',
  include_directories: include_directories('../../include'),
)

# Meson test definitions
test('unit_lexer', unit_lexer)
test('unit_parser', unit_parser)
