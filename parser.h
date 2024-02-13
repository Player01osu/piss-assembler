#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>

#include "lexer.h"

enum NodeKind {
	/*** Instructions ***/
	N_PUSH,
	N_POP,

	N_PRINT,

	N_ADD,
	N_SUB,
	N_MULT,
	N_DIV,

	N_JUMP,
	N_JUMPCMP,

	N_COPY,
	N_DUPE,
	N_SWAP,

	N_PUSHFRAME,
	N_POPFRAME,
	N_STORE,
	N_STORETOP,
	N_LOAD,

	/* Comparison instructions */
	N_CLT,
	N_CLE,
	N_CEQ,
	N_CGE,
	N_CGT,

	/***/
	N_LABEL,

	N_EOF,
	N_ILLEGAL,
};

enum TypeKind {
	TY_INT,
	TY_PTR_NAME,
};

union TypeData {
	int i;
	const char *s;
};

typedef struct Ty {
	size_t size;
	enum TypeKind kind;
} Ty;

typedef struct Sized {
	size_t size; // TODO: Use Ty
	enum TypeKind kind;
	union TypeData data;
} Sized;

typedef struct Store {
	size_t idx;
	Sized sized;
} Store;

union NodeData {
	const char *s;
	size_t n;
	ssize_t offset;
	Sized sized;
	Ty ty;
	Store store;
};

typedef struct Node {
	enum NodeKind kind;
	union NodeData data;
} Node;

typedef struct Parser {
	Lexer lexer;
	Arena *arena;
	char *error;
} Parser;

void parser_init(Parser *parser, Arena *arena, const char *src, size_t len);

Node *parser_next(Parser *parser);

#endif /* PARSER_H */
