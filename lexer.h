#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#include "arena.h"
#include "ass.h"

enum TokenKind {
#define TOK_ENUM
#define TOK(x) x,
#include "tokens.h"
#undef TOK
#undef TOK_ENUM
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

#define LEXER_BUF_SIZE (1024 * 64)
typedef struct Lexer {
	char buf[LEXER_BUF_SIZE];
	FILE *file;
	size_t len;
	size_t remaining;

	size_t prev_col;
	size_t prev_row;
	size_t col;
	size_t row;

	Arena *arena;
} Lexer;

void token_name(Token *token, char *buf);

void lexer_init(Lexer *lexer, Arena *arena, FILE *file, size_t len);

Token lexer_next(Lexer *lexer);

Span span_join(Span a, Span b);

#endif // LEXER_H
