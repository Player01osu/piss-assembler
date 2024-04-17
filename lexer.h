#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include "arena.h"

enum TokenKind {
	/*** Instructions ***/
	T_POP8,
	T_POP32,
	T_POP64,

	T_ULPUSH,
	T_ULADD,
	T_ULSUB,
	T_ULMULT,
	T_ULDIV,
	T_ULMOD,
	T_ULPRINT,

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

	T_PPUSH,
	T_PLOAD,
	T_PDEREF8,
	T_PDEREF32,
	T_PDEREF64,
	T_PDEREF,
	T_PSET8,
	T_PSET32,
	T_PSET64,
	T_PSET,

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

	T_RET,

	/* Comparison instructions */
	T_ICLT,
	T_ICLE,
	T_ICEQ,
	T_ICGT,
	T_ICGE,

	T_ULCLT,
	T_ULCLE,
	T_ULCEQ,
	T_ULCGT,
	T_ULCGE,

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
	T_SECTION_DATA,
	T_SECTION_TEXT,
	T_IDENT,

	T_UINUMLIT, // Unsigned int literal (8-bytes)
	T_INUMLIT,  // Signed int literal   (8-bytes)
	T_FNUMLIT,  // Double               (8-bytes)
	T_SLIT,     // String literal

	T_DD,
	T_DW,
	T_DB,

	T_EXTERN,

	T_COMMA,
	T_OPEN_BRACKET,
	T_CLOSE_BRACKET,
	T_EOL,
	T_EOF,
	T_ILLEGAL,
};

typedef struct Span {
	size_t start_row;
	size_t start_col;
	size_t end_row;
	size_t end_col;
} Span;

union TokenData {
	const char *s;
	uint64_t ui;
	int64_t i;
	double f;
	size_t t_size;
};

typedef struct Token {
	enum TokenKind kind;
	union TokenData data;
	Span span;
} Token;

typedef struct Lexer {
	const char *src;
	const char *cursor;

	size_t len;
	size_t remaining;

	size_t prev_col;
	size_t prev_row;
	size_t col;
	size_t row;

	Arena *arena;
} Lexer;

void token_name(Token *token, char *buf);

void lexer_init(Lexer *lexer, Arena *arena, const char *src, size_t len);

Token *lexer_next(Lexer *lexer);

Span span_join(Span a, Span b);

#endif // LEXER_H
