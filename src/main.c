#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/vm.h"

static void
print_usage(const char* prog)
{
    printf("Usage: %s [options] <program.vm>\n", prog);
    printf("Options:\n");
    printf("  -d, --debug    Enter debugger after loading\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -s, --step     Step through instructions in debug mode\n");
}

int
main(int argc, char** argv)
{
    int debug = 0;
    const char* filename = NULL;

    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
	    debug = 1;
	} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
	    print_usage(argv[0]);
	    return 0;
	} else if (filename == NULL) {
	    filename = argv[i];
	}
    }

    if (filename == NULL) {
	print_usage(argv[0]);
	return 1;
    }

    vm_state_t* vm = vm_create();
    if (vm == NULL) {
	fprintf(stderr, "Failed to create VM\n");
	return 1;
    }

    vm->exit_code = 0;

    if (vm_load(vm, filename) != 0) {
	fprintf(stderr, "Failed to load program: %s\n", filename);
	vm_destroy(vm);
	return 1;
    }

    if (debug) {
	printf("Entering debug mode...\n");
    }

    vm_run(vm);

    int exit_code = vm->exit_code;
    vm_destroy(vm);
    return exit_code;
}
