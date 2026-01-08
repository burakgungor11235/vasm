vm_sources = [
  'vm/core.c',
  'vm/registers.c',
  'vm/memory.c',
  'vm/condition.c',
  'vm/instruction.c',
  'vm/debug.c',
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
  'asm/symbol_table.c',
  'asm/error.c',
]

debug_sources = [
  'debug/debugger.c',
  'debug/cmd.c',
  'debug/disasm.c',
]

gen_script = files('../scripts/gen_lookup_tables.py')
python = find_program('python3')

gen_lookup = custom_target(
  'gen_lookup',
  input: gen_script,
  output: 'instr_name_lookup.c',
  command: [python, '@INPUT@', '--stdout'],
  capture: true,
)

gen_cond = custom_target(
  'gen_cond',
  input: gen_script,
  output: 'cond_code_lookup.c',
  command: [python, '@INPUT@', '--cond'],
  capture: true,
)

gen_header = custom_target(
  'gen_header',
  input: gen_script,
  output: 'lookup_tables.h',
  command: [python, '@INPUT@', '--header'],
  capture: true,
)

generated_asm_sources = [
  gen_header,
  gen_lookup,
  gen_cond,
]

generated_include = include_directories('.')

executable('varm',
  'main.c',
  vm_sources,
  asm_sources,
  generated_asm_sources,
  debug_sources,
  include_directories: [include_directories('../../include'), generated_include],
)

executable('vasm',
  'main_asm.c',
  asm_sources,
  generated_asm_sources,
  include_directories: [include_directories('../../include'), generated_include],
)

unit_symbol_table = executable('unit_symbol_table', 'test/unit/test_symbol_table.c',
  include_directories: include_directories('../../include'),
  sources: ['asm/symbol_table.c'],
)

unit_error = executable('unit_error', 'test/unit/test_error.c',
  include_directories: include_directories('../../include'),
  sources: ['asm/error.c'],
)

unit_lexer = executable('unit_lexer', 'test/unit/lexer.c',
  include_directories: include_directories('../../include'),
)

unit_parser = executable('unit_parser', 'test/unit/parser.c',
  include_directories: include_directories('../../include'),
)

test('unit_symbol_table', unit_symbol_table)
test('unit_error', unit_error)
test('unit_lexer', unit_lexer)
test('unit_parser', unit_parser)
