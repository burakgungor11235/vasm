#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../include/assembler.h"

int
main(int argc, char** argv)
{
    const char* input_file = NULL;
    const char* output_file = NULL;

    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
	    if (i + 1 < argc && argv[i + 1][0] != '-') {
		if (asm_debug_enable(argv[i + 1]) != 0) {
		    fprintf(stderr, "Unknown debug tag: %s\n", argv[i + 1]);
		    asm_debug_list_tags();
		    return 1;
		}
		i++;
	    } else {
		asm_debug = 1;
	    }
	} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
	    printf("Usage: %s [-d [TAG]] [-o <output.vm>] <input.asm>\n", argv[0]);
	    printf("  -d [TAG]     Enable debug output (optional TAG: LABEL, POOL, SYM, EMIT, "
	           "INSTR, ALL)\n");
	    printf("  -o <output.vm>    Output file (optional, defaults to <input>.vm)\n");
	    printf("  -h, --help   Show this help\n");
	    return 0;
	} else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
	    output_file = argv[i + 1];
	    i++;
	} else if (input_file == NULL) {
	    input_file = argv[i];
	}
    }

    if (input_file == NULL || output_file == NULL) {
	printf("Usage: %s [-d [TAG]] [-o <output.vm>] <input.asm>\n", argv[0]);
	printf("  -d [TAG]     Enable debug output (optional TAG: LABEL, POOL, SYM, EMIT, INSTR, "
	       "ALL)\n");
	printf("  -o <output.vm>    Output file (optional, defaults to <input>.vm)\n");
	return 1;
    }

    if (assemble(input_file, output_file) != 0) {
	fprintf(stderr, "Assembly failed\n");
	return 1;
    }

    return 0;
}
