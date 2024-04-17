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
	I_POP8,
	I_POP32,
	I_POP64,

	I_ULPUSH,
	I_ULADD,
	I_ULSUB,
	I_ULMULT,
	I_ULDIV,
	I_ULMOD,
	I_ULPRINT,

	I_IPUSH,
	I_IADD,
	I_ISUB,
	I_IMULT,
	I_IDIV,
	I_IMOD,
	I_IPRINT,

	I_FPUSH,
	I_FADD,
	I_FSUB,
	I_FMULT,
	I_FDIV,
	I_FPRINT,

	I_CPUSH,
	I_CADD,
	I_CSUB,
	I_CMULT,
	I_CDIV,
	I_CMOD,
	I_CPRINT,
	I_CIPRINT,

	I_PPUSH,
	I_PLOAD,
	I_PDEREF8,
	I_PDEREF32,
	I_PDEREF64,
	I_PDEREF,
	I_PSET8,
	I_PSET32,
	I_PSET64,
	I_PSET,

	I_JUMP,
	I_JUMPCMP,
	I_JUMPPROC,

	I_DUPE8,
	I_DUPE32,
	I_DUPE64,

	I_SWAP8,
	I_SWAP32,
	I_SWAP64,

	I_COPY8,
	I_COPY32,
	I_COPY64,

	I_STORE8,
	I_STORE32,
	I_STORE64,

	I_LOAD8,
	I_LOAD32,
	I_LOAD64,

	I_RET8,
	I_RET32,
	I_RET64,

	I_RET,

	/* Comparison instructions */
	I_ICLT,
	I_ICLE,
	I_ICEQ,
	I_ICGT,
	I_ICGE,

	I_ULCLT,
	I_ULCLE,
	I_ULCEQ,
	I_ULCGT,
	I_ULCGE,

	I_FCLT,
	I_FCLE,
	I_FCEQ,
	I_FCGT,
	I_FCGE,

	I_CCLT,
	I_CCLE,
	I_CCEQ,
	I_CCGT,
	I_CCGE,
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
