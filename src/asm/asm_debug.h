#ifndef ASM_DEBUG_H
#define ASM_DEBUG_H

#include <stdio.h>
#include <string.h>

extern int asm_debug;

int
asm_debug_enable(const char* tag);
void
asm_debug_disable_all(void);
void
asm_debug_list_tags(void);

#define MAX_DEBUG_TAGS 16
extern int asm_debug_tags[MAX_DEBUG_TAGS];
extern const char* asm_debug_tag_names[MAX_DEBUG_TAGS];
extern int asm_debug_tag_count;

static inline int
asm_debug_is_enabled(const char* tag)
{
    for (int i = 0; i < asm_debug_tag_count; i++) {
	if (asm_debug_tag_names[i] != NULL && strcmp(asm_debug_tag_names[i], tag) == 0) {
	    return asm_debug_tags[i];
	}
    }
    return 0;
}

#define ASM_DEBUG(fmt, ...)                                                                        \
    do {                                                                                           \
	if (asm_debug) {                                                                           \
	    fprintf(stderr, "[ASM] " fmt, ##__VA_ARGS__);                                          \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_LABEL(fmt, ...)                                                                  \
    do {                                                                                           \
	if (asm_debug_is_enabled("LABEL")) {                                                       \
	    fprintf(stderr, "[LABEL] " fmt, ##__VA_ARGS__);                                        \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_POOL(fmt, ...)                                                                   \
    do {                                                                                           \
	if (asm_debug_is_enabled("POOL")) {                                                        \
	    fprintf(stderr, "[POOL] " fmt, ##__VA_ARGS__);                                         \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_SYM(fmt, ...)                                                                    \
    do {                                                                                           \
	if (asm_debug_is_enabled("SYM")) {                                                         \
	    fprintf(stderr, "[SYM] " fmt, ##__VA_ARGS__);                                          \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_EMIT(fmt, ...)                                                                   \
    do {                                                                                           \
	if (asm_debug_is_enabled("EMIT")) {                                                        \
	    fprintf(stderr, "[EMIT] " fmt, ##__VA_ARGS__);                                         \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_INSTR(fmt, ...)                                                                  \
    do {                                                                                           \
	if (asm_debug_is_enabled("INSTR")) {                                                       \
	    fprintf(stderr, "[INSTR] " fmt, ##__VA_ARGS__);                                        \
	}                                                                                          \
    } while (0)

#endif
