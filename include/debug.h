#ifndef DEBUG_H
#define DEBUG_H

#include "opcode.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>

#define MAX_TAGS 32

typedef enum { SEV_ERROR = 1, SEV_WARN, SEV_INFO, SEV_DEBUG, SEV_TRACE } severity_t;

typedef struct {
    const char* name;
    const char* abbr;
    const char* description;
    int enabled;
    int severity;
} debug_tag_t;

typedef struct {
    int verbosity;
    debug_tag_t tags[MAX_TAGS];
    int tag_count;
    int use_color;
    FILE* output;
    int exit_code;
    uint64_t instr_count;
    uint64_t mem_reads;
    uint64_t mem_writes;
    uint64_t syscalls;
} debug_config_t;

static inline void
debug_log(debug_config_t* config, const char* tag, const char* format, ...)
{
    if (!config)
	return;
    for (int i = 0; i < config->tag_count; i++) {
	if ((config->tags[i].name && strcasecmp(config->tags[i].name, tag) == 0) ||
	    (config->tags[i].abbr && strcasecmp(config->tags[i].abbr, tag) == 0)) {
	    if (config->tags[i].enabled) {
		va_list args;
		va_start(args, format);
		vfprintf(config->output, format, args);
		va_end(args);
	    }
	    return;
	}
    }
}

void
debug_init(debug_config_t* config);
void
debug_destroy(debug_config_t* config);

int
debug_set_verbosity(debug_config_t* config, int level);
int
debug_enable_tag(debug_config_t* config, const char* tag);
int
debug_disable_tag(debug_config_t* config, const char* tag);
int
debug_is_enabled(debug_config_t* config, const char* tag);
void
debug_list_tags(debug_config_t* config);

void
debug_instr(debug_config_t* config, vm_state_t* vm, u32 instr, u32 pc);
void
debug_regs(debug_config_t* config, vm_state_t* vm);
void
debug_mem(debug_config_t* config, u32 addr, u32 value, int is_read);
void
debug_syscall(debug_config_t* config, vm_state_t* vm, u32 syscall);
void
debug_stats(debug_config_t* config, vm_state_t* vm);
void
debug_load(debug_config_t* config, const char* filename, u32 text_offset, u32 text_size,
           u8* text_data);

#endif
