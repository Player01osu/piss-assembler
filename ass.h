#ifndef ASS_H
#define ASS_H

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "parser.h"
#include "lexer.h"

#define FERROR stderr
#define panic(...) \
	do { \
		fprintf(FERROR, "%s:%d:", __FILE__, __LINE__); \
		fprintf(FERROR, __VA_ARGS__); \
		exit(1); \
	} while (0)
#define BUF_SIZE 1024 * 4
#define STACK_SIZE 1024 * 16
#define LOCAL_SIZE 256

enum InstructionKind {
#define INSTR(x, _) I_##x,
#include "instructions.h"
#undef INSTR
};

enum DataSize {
	SIZE_DD,
	SIZE_DW,
	SIZE_DB,
};

typedef unsigned char byte;

typedef struct Label {
	const char *name;
	size_t location;
} Label;

typedef struct LabelMap {
	Label *labels;
	size_t cap;
	size_t len;
} LabelMap;

#endif
