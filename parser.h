#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>

#include "ass.h"
#include "lexer.h"

enum NodeKind {
	N_INSTRUCTION,
	N_LABEL,
	N_DECLARATION,

	N_EOF,
};

enum InstructionKind {
#define INSTR(x, _) I_##x,
#include "instructions.h"
#undef INSTR
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

enum ParserState {
	PARSE_DATA,
	PARSE_TEXT,
};

union LitData {
	const char *s;
	uint64_t ui;
	int64_t i;
	double f;
};

typedef struct Lit {
	enum LitKind kind;
	union LitData data;
	Span span;
} Lit;

union ProcLocation {
	const char *s;
	ssize_t offset;
};

typedef struct Proc {
	union ProcLocation location;
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

union InstructionData {
	const char *s;
	void *ptr;
	size_t n;
	ssize_t offset;
	Lit lit;
	Proc proc;
};

typedef struct Instruction {
	enum InstructionKind kind;
	union InstructionData data;
} Instruction;

union NodeData {
	const char *s;
	Instruction instruction;
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
	Span span;
	enum ParserState state;
} Parser;

void parser_init(Parser *parser, Arena *arena, FILE *file, size_t len);

Node *parser_next(Parser *parser);

#endif /* PARSER_H */
