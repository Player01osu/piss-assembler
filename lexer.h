#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include "arena.h"

enum TokenKind {
	/*** Instructions ***/
	T_POP8,
	T_POP32,
	T_POP64,

	T_IPUSH,
	T_IADD,
	T_ISUB,
	T_IMULT,
	T_IDIV,
	T_IMOD,
	T_IPRINT,

	T_FPUSH,
	T_FADD,
	T_FSUB,
	T_FMULT,
	T_FDIV,
	T_FPRINT,

	T_CPUSH,
	T_CADD,
	T_CSUB,
	T_CMULT,
	T_CDIV,
	T_CMOD,
	T_CPRINT,
	T_CIPRINT,

	T_JUMP,
	T_JUMPCMP,
	T_JUMPPROC,

	T_DUPE8,
	T_DUPE32,
	T_DUPE64,

	T_SWAP8,
	T_SWAP32,
	T_SWAP64,

	T_COPY8,
	T_COPY32,
	T_COPY64,

	T_STORE8,
	T_STORE32,
	T_STORE64,

	T_LOAD8,
	T_LOAD32,
	T_LOAD64,

	T_RET8,
	T_RET32,
	T_RET64,

	/* Comparison instructions */
	T_ICLT,
	T_ICLE,
	T_ICEQ,
	T_ICGT,
	T_ICGE,

	T_FCLT,
	T_FCLE,
	T_FCEQ,
	T_FCGT,
	T_FCGE,

	T_CCLT,
	T_CCLE,
	T_CCEQ,
	T_CCGT,
	T_CCGE,

	/***/

	T_DATATYPE,
	T_LABEL,
	T_IDENT,

	T_UINUMLIT, // Unsigned int literal (8-bytes)
	T_INUMLIT,  // Signed int literal   (8-bytes)
	T_FNUMLIT,  // Double               (8-bytes)

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
	P_FLOAT,
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
	uint64_t ui;
	int64_t i;
	double f;
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

void token_name(Token *token, char *buf);

void lexer_init(Lexer *lexer, Arena *arena, const char *src, size_t len);

Token *lexer_next(Lexer *lexer);

#endif // LEXER_H
