#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>

#include "ass.h"
#include "lexer.h"

enum NodeKind {
#define INSTR(x, _) N_##x,
#include "instructions.h"
#undef INSTR
	/***/
	N_LABEL,
	N_DECLARATION,

	N_EOF,
	N_ILLEGAL,
};

enum DeclarationKind {
	D_EXTERN,
	D_DD,
	D_DW,
	D_DB,
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

typedef struct Declaration {
	enum DeclarationKind kind;
	const char *ident;
	void *bytes;
	size_t len;
	Span span;
} Declaration;

typedef struct DeclarationMap {
	Declaration *declarations;
	size_t cap;
	size_t len;
} DeclarationMap;

union NodeData {
	const char *s;
	void *ptr;
	size_t n;
	ssize_t offset;
	Lit lit;
	Proc proc;
	Declaration declaration;
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

	enum {
		PARSE_DATA,
		PARSE_TEXT,
	} state;
} Parser;

void parser_init(Parser *parser, Arena *arena, const char *src, size_t len);

Node *parser_next(Parser *parser);

#endif /* PARSER_H */
