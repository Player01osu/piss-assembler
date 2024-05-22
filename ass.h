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

#include "arena.h"

#define FERROR stderr
#define panic(...)                                             \
	do {                                                   \
		fprintf(FERROR, "%s:%d:", __FILE__, __LINE__); \
		fprintf(FERROR, __VA_ARGS__);                  \
		abort();                                       \
	} while (0)

#ifdef DEBUG_TRACE
#define dprintf(...)                                                 \
	do {                                                         \
		fprintf(FERROR, "%s:%d:DEBUG:", __FILE__, __LINE__); \
		fprintf(FERROR, __VA_ARGS__);                        \
	} while (0)
#else
#define dprintf(...) ;
#endif
#define BUF_SIZE (1024 * 4)
#define STACK_SIZE (1024 * 16)
#define LOCAL_SIZE 256

enum DataSize {
	SIZE_DD,
	SIZE_DW,
	SIZE_DB,
};

typedef unsigned char byte;

union Data {
	const char *s;
	uint64_t ui;
	int64_t i;
	float f;
	double d;
};

typedef struct Label {
	const char *name;
	size_t location;
} Label;

typedef struct LabelMap {
	Label *labels;
	size_t cap;
	size_t len;
} LabelMap;

#define arena_xalloc(arena, size) _arena_xalloc(__FILE__, __LINE__, arena, size)
#define xmalloc(size) _xmalloc(__FILE__, __LINE__, size)
#define xrealloc(ptr, size) _xrealloc(__FILE__, __LINE__, ptr, size)

void *_arena_xalloc(char *filename, int row, Arena *arena, size_t size);

void *_xmalloc(char *filename, int row, size_t size);

void *_xrealloc(char *filename, int row, void *ptr, size_t size);

#endif
