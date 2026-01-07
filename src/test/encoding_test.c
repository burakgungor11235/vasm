#include <stdio.h>
#include <string.h>
#include "../../include/vm.h"
#include "../../include/assembler.h"
#include "../../include/opcode.h"

int main() {
    token_t tokens[256];
    program_state_t prog;
    
    const char *src = "mov r0, #42";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);
    
    printf("Instruction: 0x%08X\n", prog.text[0]);
    printf("Opcode: 0x%02X (expected 0x%02X for MOV)\n", prog.text[0] >> 24, OP_MOV);
    printf("Condition: 0x%X (expected 0xE for AL)\n", (prog.text[0] >> 20) & 0xF);
    printf("RD: 0x%X (expected 0)\n", (prog.text[0] >> 12) & 0xF);
    printf("Operand: 0x%X\n", prog.text[0] & 0xFFF);
    
    return 0;
}
