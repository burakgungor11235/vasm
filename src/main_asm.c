#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/assembler.h"

int
main(int argc, char** argv)
{
    const char* input_file = NULL;
    const char* output_file = NULL;

    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
	    output_file = argv[i + 1];
	    i++;
	} else if (input_file == NULL) {
	    input_file = argv[i];
	}
    }

    if (input_file == NULL || output_file == NULL) {
	printf("Usage: %s [-o <output.vm>] <input.asm>\n", argv[0]);
	printf("  -o <output.vm>    Output file (optional, defaults to <input>.vm)\n");
	return 1;
    }

    if (assemble(input_file, output_file) != 0) {
	fprintf(stderr, "Assembly failed\n");
	return 1;
    }

    return 0;
}
