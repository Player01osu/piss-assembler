#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include "arena.h"

enum TokenKind {
	/*** Instructions ***/
	T_PUSH,
	T_POP,

	T_PRINT,

	T_ADD,
	T_SUB,
	T_MULT,
	T_DIV,

	T_JUMP,
	T_JUMPCMP,

	T_COPY,
	T_DUPE,
	T_SWAP,

	T_PUSHFRAME,
	T_POPFRAME,
	T_STORE,
	T_STORETOP,
	T_LOAD,

	/* Comparison instructions */
	T_CLT,
	T_CLE,
	T_CEQ,
	T_CGE,
	T_CGT,

	/***/

	T_DATATYPE,
	T_LABEL,
	T_IDENT,
	T_NUMLIT,

	T_COMMA,
	T_EOL,
	T_EOF,
	T_ILLEGAL,
};

enum DatatypeKind {
	TY_PRIMATIVE,
	TY_CUSTOM,
};

enum Primative {
	P_INT,
};

typedef struct Datatype {
	enum DatatypeKind kind;
	union {
		enum Primative primative;
		char *s;
	} data;
} Datatype;

union TokenData {
	const char *s;
	int i;
	size_t t_size;
	Datatype datatype;
};

typedef struct Token {
	enum TokenKind kind;
	union TokenData data;
	// TODO: Add span
} Token;

typedef struct Lexer {
	const char *src;
	const char *cursor;

	size_t len;
	size_t remaining;

	size_t col;
	size_t row;

	Arena *arena;
} Lexer;

void lexer_init(Lexer *lexer, Arena *arena, const char *src, size_t len);

Token *lexer_next(Lexer *lexer);

#endif // LEXER_H
