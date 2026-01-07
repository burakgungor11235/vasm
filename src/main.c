#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/vm.h"
#include "../include/debug.h"

static void
print_usage(const char* prog)
{
    printf("Usage: %s [options] <program.varm>\n", prog);
    printf("Options:\n");
    printf("  -v, --verbose <level>  Set verbosity level (0-5)\n");
    printf("  -d, --debug <tag>      Enable debug tag\n");
    printf("  --no-<tag>             Disable a debug tag\n");
    printf("  --tags                 List available debug tags\n");
    printf("  -h, --help             Show this help message\n");
}

int
main(int argc, char** argv)
{
    const char* filename = NULL;
    debug_config_t debug_config;
    int list_tags = 0;

    debug_init(&debug_config);

    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
	    print_usage(argv[0]);
	    return 0;
	} else if (strcmp(argv[i], "--tags") == 0) {
	    list_tags = 1;
	} else if ((strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) &&
	           i + 1 < argc) {
	    int level = atoi(argv[i + 1]);
	    if (level < 0 || level > 5) {
		fprintf(stderr, "Error: verbosity level must be 0-5\n");
		return 1;
	    }
	    debug_set_verbosity(&debug_config, level);
	    i++;
	} else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
	    if (i + 1 >= argc) {
		fprintf(stderr, "Error: -d requires a tag name\n");
		return 1;
	    }
	    if (debug_enable_tag(&debug_config, argv[i + 1]) != 0) {
		fprintf(stderr, "Error: unknown tag '%s'\n", argv[i + 1]);
		return 1;
	    }
	    i++;
	} else if (strncmp(argv[i], "--no-", 5) == 0) {
	    const char* tag = argv[i] + 5;
	    if (debug_disable_tag(&debug_config, tag) != 0) {
		fprintf(stderr, "Error: unknown tag '%s'\n", tag);
		return 1;
	    }
	} else if (filename == NULL) {
	    filename = argv[i];
	} else {
	    fprintf(stderr, "Error: unexpected argument '%s'\n", argv[i]);
	    return 1;
	}
    }

    if (list_tags) {
	debug_list_tags(&debug_config);
	return 0;
    }

    if (filename == NULL) {
	print_usage(argv[0]);
	return 1;
    }

    vm_state_t* vm = vm_create();
    if (vm == NULL) {
	fprintf(stderr, "Error: failed to create VM\n");
	return 1;
    }

    vm->exit_code = 0;
    vm->debug_config = &debug_config;

    if (vm_load(vm, filename) != 0) {
	fprintf(stderr, "Error: failed to load program: %s\n", filename);
	vm_destroy(vm);
	return 1;
    }

    vm_run(vm);

    debug_stats(&debug_config, vm);

    int exit_code = vm->exit_code;
    vm_destroy(vm);
    return exit_code;
}
