#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: %s <input.asm> <output.vm>\n", argv[0]);
    return 1;
  }

  printf("Assembler: %s -> %s\n", argv[1], argv[2]);
  printf("Assembler not yet implemented.\n");
  return 0;
}
