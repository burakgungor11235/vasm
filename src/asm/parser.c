#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "../../include/vm.h"
#include "../../include/assembler.h"

#define MAX_LABELS 256
#define MAX_INSTRUCTIONS 1024

typedef struct {
  char name[64];
  u32 address;
} label_t;

typedef struct {
  u32 address;
  char name[64];
  int is_branch;
} reloc_t;

typedef struct {
  u32 text[4096];
  u32 text_size;
  u8 data[4096];
  u32 data_size;
  label_t labels[MAX_LABELS];
  int label_count;
  reloc_t relocs[MAX_LABELS];
  int reloc_count;
  u32 current_addr;
  int in_text_section;
} program_state_t;

static int lookup_label(program_state_t *prog, const char *name) {
  for (int i = 0; i < prog->label_count; i++) {
    if (strcmp(prog->labels[i].name, name) == 0) {
      return prog->labels[i].address;
    }
  }
  return -1;
}

static void add_label(program_state_t *prog, const char *name, u32 addr) {
  if (prog->label_count < MAX_LABELS) {
    strncpy(prog->labels[prog->label_count].name, name, 63);
    prog->labels[prog->label_count].address = addr;
    prog->label_count++;
  }
}

static void add_reloc(program_state_t *prog, const char *name, u32 addr, int branch) {
  if (prog->reloc_count < MAX_LABELS) {
    strncpy(prog->relocs[prog->reloc_count].name, name, 63);
    prog->relocs[prog->reloc_count].address = addr;
    prog->relocs[prog->reloc_count].is_branch = branch;
    prog->reloc_count++;
  }
}

static int get_register(const char *name) {
  if (strlen(name) == 2 && name[0] == 'r' && isdigit(name[1])) {
    return name[1] - '0';
  }
  if (strlen(name) == 3 && name[0] == 'r' && isdigit(name[1]) && isdigit(name[2])) {
    return (name[1] - '0') * 10 + (name[2] - '0');
  }
  if (strcasecmp(name, "sp") == 0) return 13;
  if (strcasecmp(name, "lr") == 0) return 14;
  if (strcasecmp(name, "pc") == 0) return 15;
  return -1;
}

static int parse_condition(const char *cond) {
  if (cond == NULL || cond[0] == '\0') return COND_AL;

  if (strcasecmp(cond, "eq") == 0) return COND_EQ;
  if (strcasecmp(cond, "ne") == 0) return COND_NE;
  if (strcasecmp(cond, "cs") == 0 || strcasecmp(cond, "hs") == 0) return COND_CS;
  if (strcasecmp(cond, "cc") == 0 || strcasecmp(cond, "lo") == 0) return COND_CC;
  if (strcasecmp(cond, "mi") == 0) return COND_MI;
  if (strcasecmp(cond, "pl") == 0) return COND_PL;
  if (strcasecmp(cond, "vs") == 0) return COND_VS;
  if (strcasecmp(cond, "vc") == 0) return COND_VC;
  if (strcasecmp(cond, "hi") == 0) return COND_HI;
  if (strcasecmp(cond, "ls") == 0) return COND_LS;
  if (strcasecmp(cond, "ge") == 0) return COND_GE;
  if (strcasecmp(cond, "lt") == 0) return COND_LT;
  if (strcasecmp(cond, "gt") == 0) return COND_GT;
  if (strcasecmp(cond, "le") == 0) return COND_LE;
  return COND_AL;
}

static int get_opcode(const char *name) {
  if (strcasecmp(name, "mov") == 0) return OP_MOV;
  if (strcasecmp(name, "mvn") == 0) return OP_MVN;
  if (strcasecmp(name, "add") == 0) return OP_ADD;
  if (strcasecmp(name, "adc") == 0) return OP_ADC;
  if (strcasecmp(name, "sub") == 0) return OP_SUB;
  if (strcasecmp(name, "sbc") == 0) return OP_SBC;
  if (strcasecmp(name, "rsb") == 0) return OP_RSB;
  if (strcasecmp(name, "rsc") == 0) return OP_RSC;
  if (strcasecmp(name, "and") == 0) return OP_AND;
  if (strcasecmp(name, "eor") == 0) return OP_EOR;
  if (strcasecmp(name, "orr") == 0) return OP_ORR;
  if (strcasecmp(name, "bic") == 0) return OP_BIC;
  if (strcasecmp(name, "cmp") == 0) return OP_CMP;
  if (strcasecmp(name, "cmn") == 0) return OP_CMN;
  if (strcasecmp(name, "tst") == 0) return OP_TST;
  if (strcasecmp(name, "teq") == 0) return OP_TEQ;
  if (strcasecmp(name, "mul") == 0) return OP_MUL;
  if (strcasecmp(name, "mla") == 0) return OP_MLA;
  if (strcasecmp(name, "ldr") == 0) return OP_LDR;
  if (strcasecmp(name, "ldrb") == 0) return OP_LDRB;
  if (strcasecmp(name, "str") == 0) return OP_STR;
  if (strcasecmp(name, "strb") == 0) return OP_STRB;
  if (strcasecmp(name, "b") == 0) return OP_B;
  if (strcasecmp(name, "bl") == 0) return OP_BL;
  if (strcasecmp(name, "bx") == 0) return OP_BX;
  if (strcasecmp(name, "halt") == 0) return OP_HALT;
  if (strcasecmp(name, "swi") == 0) return OP_SWI;
  if (strcasecmp(name, "nop") == 0) return OP_NOP;
  return -1;
}

static u32 parse_immediate(const char *value) {
  u32 result = 0;

  if (strncasecmp(value, "0x", 2) == 0) {
    sscanf(value + 2, "%x", &result);
  } else if (strncasecmp(value, "0b", 2) == 0) {
    result = strtoul(value + 2, NULL, 2);
  } else {
    result = strtoul(value, NULL, 10);
  }

  u8 rotate = 0;
  while (rotate < 16 && (result & 0xFF000000)) {
    result = (result >> 2) | ((result & 0x3) << 30);
    rotate++;
  }

  return (rotate << 8) | (result & 0xFF);
}

static int parse_shift(const char *s, int *shift_type, int *shift_imm) {
  if (strstr(s, "lsl") != NULL) {
    *shift_type = 0;
    if (sscanf(strstr(s, "lsl") + 3, "%d", shift_imm) != 1) {
      *shift_imm = 0;
    }
    return 1;
  }
  if (strstr(s, "lsr") != NULL) {
    *shift_type = 1;
    if (sscanf(strstr(s, "lsr") + 3, "%d", shift_imm) != 1) {
      *shift_imm = 0;
    }
    return 1;
  }
  if (strstr(s, "asr") != NULL) {
    *shift_type = 2;
    if (sscanf(strstr(s, "asr") + 3, "%d", shift_imm) != 1) {
      *shift_imm = 0;
    }
    return 1;
  }
  if (strstr(s, "ror") != NULL) {
    *shift_type = 3;
    if (sscanf(strstr(s, "ror") + 3, "%d", shift_imm) != 1) {
      *shift_imm = 0;
    }
    return 1;
  }
  return 0;
}

static void emit_instr(program_state_t *prog, u32 instr) {
  if (prog->in_text_section && prog->text_size < 4096) {
    prog->text[prog->text_size++] = instr;
  }
  prog->current_addr += 4;
}

static int is_label_reference(const char *s) {
  return (isalpha(s[0]) || s[0] == '_') && strpbrk(s, "#[],") == NULL;
}

int parse(token_t *tokens, int token_count, program_state_t *prog) {
  int i = 0;
  prog->text_size = 0;
  prog->data_size = 0;
  prog->label_count = 0;
  prog->reloc_count = 0;
  prog->current_addr = 0;
  prog->in_text_section = 1;

  for (i = 0; i < token_count && tokens[i].type != TOKEN_EOF; i++) {
    if (tokens[i].type == TOKEN_NEWLINE) {
      continue;
    }

    if (tokens[i].type == TOKEN_LABEL) {
      add_label(prog, tokens[i].value, prog->current_addr);
      continue;
    }

    if (tokens[i].type == TOKEN_DIRECTIVE) {
      char *dir = tokens[i].value;
      i++;

      if (strcasecmp(dir, ".text") == 0) {
        prog->in_text_section = 1;
        continue;
      }

      if (strcasecmp(dir, ".data") == 0) {
        prog->in_text_section = 0;
        prog->current_addr = 0x10000;
        continue;
      }

      if (strcasecmp(dir, ".word") == 0) {
        while (i < token_count && tokens[i].type != TOKEN_NEWLINE && tokens[i].type != TOKEN_EOF) {
          if (tokens[i].type == TOKEN_IMMEDIATE) {
            u32 val = strtoul(tokens[i].value, NULL, 0);
            if (prog->data_size < 4096) {
              prog->data[prog->data_size++] = val & 0xFF;
              prog->data[prog->data_size++] = (val >> 8) & 0xFF;
              prog->data[prog->data_size++] = (val >> 16) & 0xFF;
              prog->data[prog->data_size++] = (val >> 24) & 0xFF;
              prog->current_addr += 4;
            }
          }
          i++;
        }
        continue;
      }

      if (strcasecmp(dir, ".byte") == 0) {
        while (i < token_count && tokens[i].type != TOKEN_NEWLINE && tokens[i].type != TOKEN_EOF) {
          if (tokens[i].type == TOKEN_IMMEDIATE) {
            u32 val = strtoul(tokens[i].value, NULL, 0);
            if (prog->data_size < 4096) {
              prog->data[prog->data_size++] = val & 0xFF;
              prog->current_addr++;
            }
          }
          i++;
        }
        continue;
      }

      if (strcasecmp(dir, ".equ") == 0 || strcasecmp(dir, ".set") == 0) {
        if (i + 2 < token_count && tokens[i + 1].type == TOKEN_IDENTIFIER) {
          u32 val = strtoul(tokens[i + 2].value, NULL, 0);
          add_label(prog, tokens[i + 1].value, val);
        }
        while (i < token_count && tokens[i].type != TOKEN_NEWLINE && tokens[i].type != TOKEN_EOF) i++;
        continue;
      }

      continue;
    }

    if (tokens[i].type == TOKEN_INSTRUCTION) {
      char *instr_name = tokens[i].value;
      char *condition = NULL;
      char *dot = strchr(instr_name, '.');
      if (dot != NULL) {
        *dot = '\0';
        condition = dot + 1;
      }

      int opcode = get_opcode(instr_name);
      i++;

      if (opcode < 0) {
        continue;
      }

      u32 instr = 0;
      u8 rd = 0, rn = 0;
      u32 operand = 0;
      u32 offset = 0;

      if (opcode == OP_B || opcode == OP_BL) {
        if (i < token_count && tokens[i].type == TOKEN_IDENTIFIER) {
          add_reloc(prog, tokens[i].value, prog->current_addr, opcode == OP_BL || opcode == OP_B);
          offset = 0;
          i++;
        } else if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
          offset = strtoul(tokens[i].value, NULL, 0);
          i++;
        }
        instr = (opcode << 24) | (parse_condition(condition) << 20) | (offset & 0xFFFFFF);
        emit_instr(prog, instr);
        continue;
      }

      if (opcode == OP_HALT || opcode == OP_NOP) {
        instr = (opcode << 24) | (parse_condition(condition) << 20);
        emit_instr(prog, instr);
        continue;
      }

      if (opcode == OP_SWI) {
        if (i < token_count && tokens[i].type == TOKEN_HASH) {
          i++;
        }
        if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
          offset = strtoul(tokens[i].value, NULL, 0) & 0xFFFFFF;
          i++;
        }
        instr = (opcode << 24) | (parse_condition(condition) << 20) | offset;
        emit_instr(prog, instr);
        continue;
      }

      if (opcode == OP_MOV || opcode == OP_MVN) {
        if (i < token_count && tokens[i].type == TOKEN_IDENTIFIER) {
          rd = get_register(tokens[i].value);
          i++;
        }
        if (i < token_count && tokens[i].type == TOKEN_COMMA) {
          i++;
        }
        if (i < token_count) {
          if (tokens[i].type == TOKEN_HASH) {
            i++;
            if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
              operand = parse_immediate(tokens[i].value);
              i++;
            }
          } else if (tokens[i].type == TOKEN_IMMEDIATE) {
            operand = parse_immediate(tokens[i].value);
            i++;
          } else if (tokens[i].type == TOKEN_IDENTIFIER) {
            operand = parse_immediate(tokens[i].value);
            i++;
          }
        }
        instr = (opcode << 24) | (parse_condition(condition) << 20) | (rd << 12) | operand;
        emit_instr(prog, instr);
        continue;
      }

      if (opcode == OP_CMP || opcode == OP_CMN || opcode == OP_TST || opcode == OP_TEQ) {
        if (i < token_count) {
          rn = get_register(tokens[i].value);
          i++;
        }
        if (i < token_count && (tokens[i].type == TOKEN_COMMA)) {
          i++;
          if (i < token_count) {
            if (tokens[i].type == TOKEN_HASH) {
              i++;
              if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
                operand = parse_immediate(tokens[i].value);
              }
            } else if (tokens[i].type == TOKEN_IDENTIFIER) {
              operand = parse_immediate(tokens[i].value);
            }
          }
        }
        instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) | operand;
        emit_instr(prog, instr);
        continue;
      }

      if (opcode == OP_MUL) {
        u8 rm = 0;
        if (i < token_count) { rd = get_register(tokens[i].value); i++; }
        if (i < token_count && tokens[i].type == TOKEN_COMMA) { i++; }
        if (i < token_count) { rn = get_register(tokens[i].value); i++; }
        if (i < token_count && tokens[i].type == TOKEN_COMMA) { i++; }
        if (i < token_count) { rm = get_register(tokens[i].value); i++; }
        instr = (opcode << 24) | (parse_condition(condition) << 20) | (rd << 16) | (rn << 8) | rm;
        emit_instr(prog, instr);
        continue;
      }

      if (i < token_count) {
        rd = get_register(tokens[i].value);
        i++;
      }

      if (i < token_count && tokens[i].type == TOKEN_COMMA) {
        i++;
      }

      if (i < token_count) {
        rn = get_register(tokens[i].value);
        i++;
      }

      if (i < token_count && tokens[i].type == TOKEN_COMMA) {
        i++;
      }

      if (i < token_count) {
        if (tokens[i].type == TOKEN_HASH) {
          i++;
          if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
            operand = parse_immediate(tokens[i].value);
            i++;
          }
        } else if (tokens[i].type == TOKEN_IMMEDIATE) {
          operand = parse_immediate(tokens[i].value);
          i++;
        } else if (tokens[i].type == TOKEN_IDENTIFIER) {
          operand = parse_immediate(tokens[i].value);
          i++;
        } else if (tokens[i].type == TOKEN_LBRACKET) {
          u32 base = rn;
          u32 off = 0;
          i++;
          if (i < token_count && tokens[i].type == TOKEN_HASH) {
            i++;
            if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
              off = strtoul(tokens[i].value, NULL, 0);
              i++;
            }
          }
          if (i < token_count && tokens[i].type == TOKEN_RBRACKET) {
            i++;
          }
          offset = off & 0xFFF;
          rn = base;
          operand = offset;
          instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) | (rd << 12) | operand;
          emit_instr(prog, instr);
          continue;
        }
      }

      instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) | (rd << 12) | operand;
      emit_instr(prog, instr);
    }
  }

  for (int j = 0; j < prog->reloc_count; j++) {
    int addr = lookup_label(prog, prog->relocs[j].name);
    if (addr >= 0) {
      u32 *patch_addr = &prog->text[prog->relocs[j].address / 4];
      if (prog->relocs[j].is_branch) {
        u32 offset = (addr - prog->relocs[j].address - 8) / 4;
        *patch_addr = (*patch_addr & 0xFF000000) | (offset & 0xFFFFFF);
      } else {
        *patch_addr = addr;
      }
    }
  }

  return 0;
}

int write_vm_file(program_state_t *prog, const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (f == NULL) {
    return -1;
  }

  char header[32] = {'V', 'A', 'R', 'M'};
  u32 text_offset = 32;
  u32 data_offset = text_offset + prog->text_size * 4;
  u32 entry = 0;

  *(u32 *)&header[4] = text_offset;
  *(u32 *)&header[8] = prog->text_size * 4;
  *(u32 *)&header[12] = data_offset;
  *(u32 *)&header[16] = prog->data_size;
  *(u32 *)&header[20] = entry;
  *(u32 *)&header[24] = 0;
  *(u32 *)&header[28] = 0;

  fwrite(header, 1, 32, f);
  fwrite(prog->text, 4, prog->text_size, f);
  fwrite(prog->data, 1, prog->data_size, f);

  fclose(f);
  return 0;
}

int assemble(const char *input_file, const char *output_file) {
  FILE *f = fopen(input_file, "r");
  if (f == NULL) {
    fprintf(stderr, "Cannot open file: %s\n", input_file);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buffer = malloc(size + 1);
  if (buffer == NULL) {
    fclose(f);
    return -1;
  }

  fread(buffer, 1, size, f);
  buffer[size] = '\0';
  fclose(f);

  token_t tokens[4096];
  int token_count = tokenize(buffer, tokens, 4096);
  free(buffer);

  program_state_t prog;
  memset(&prog, 0, sizeof(prog));

  parse(tokens, token_count, &prog);
  write_vm_file(&prog, output_file);

  printf("Assembled: %lu bytes of code, %lu bytes of data\n",
         (unsigned long)prog.text_size * 4, (unsigned long)prog.data_size);
  printf("Labels: %d\n", prog.label_count);

  return 0;
}
