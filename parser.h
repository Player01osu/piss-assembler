#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>

#include "lexer.h"

enum NodeKind {
	/*** Instructions ***/
	N_POP8,
	N_POP32,
	N_POP64,

	N_IPUSH,
	N_IADD,
	N_ISUB,
	N_IMULT,
	N_IDIV,
	N_IMOD,
	N_IPRINT,

	N_FPUSH,
	N_FADD,
	N_FSUB,
	N_FMULT,
	N_FDIV,
	N_FPRINT,

	N_CPUSH,
	N_CADD,
	N_CSUB,
	N_CMULT,
	N_CDIV,
	N_CMOD,
	N_CPRINT,
	N_CIPRINT,

	N_JUMP,
	N_JUMPCMP,
	N_JUMPPROC,

	N_DUPE8,
	N_DUPE32,
	N_DUPE64,

	N_SWAP8,
	N_SWAP32,
	N_SWAP64,

	N_COPY8,
	N_COPY32,
	N_COPY64,

	N_STORE8,
	N_STORE32,
	N_STORE64,

	N_LOAD8,
	N_LOAD32,
	N_LOAD64,

	N_RET8,
	N_RET32,
	N_RET64,

	N_RET,

	/* Comparison instructions */
	N_ICLT,
	N_ICLE,
	N_ICEQ,
	N_ICGT,
	N_ICGE,

	N_FCLT,
	N_FCLE,
	N_FCEQ,
	N_FCGT,
	N_FCGE,

	N_CCLT,
	N_CCLE,
	N_CCEQ,
	N_CCGT,
	N_CCGE,

	/***/
	N_LABEL,

	N_EOF,
	N_ILLEGAL,
};

enum LitKind {
	L_INT,
	L_UINT,
	L_FLOAT,
	L_PTR,
};

typedef struct Lit {
	enum LitKind kind;
	union {
		const char *s;
		uint64_t ui;
		int64_t i;
		double f;
	} data;
	Span span;
} Lit;

typedef struct Proc {
	union {
		const char *s;
		ssize_t offset;
	} location;

	size_t argc;
	Span span;
} Proc;

union NodeData {
	const char *s;
	size_t n;
	ssize_t offset;
	Lit lit;
	Proc proc;
};

typedef struct Node {
	enum NodeKind kind;
	union NodeData data;
	Span span;
} Node;

typedef struct Parser {
	Lexer lexer;
	Arena *arena;
	char *error;
} Parser;

void parser_init(Parser *parser, Arena *arena, const char *src, size_t len);

Node *parser_next(Parser *parser);

#endif /* PARSER_H */
