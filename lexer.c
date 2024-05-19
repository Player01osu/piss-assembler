#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ass.h"
#include "lexer.h"

#define span_dbg_print(span) _span_dbg_print(__FILE__, __LINE__, span)

void _span_dbg_print(char *filename, int row, Span span)
{
	fprintf(stderr, "%s:%d:SPAN: %lu:%lu => %lu:%lu\n", filename, row, span.start_row, span.start_col, span.end_row, span.end_col);
}

Span span_join(Span a, Span b)
{
	assert(a.start_row < b.start_row || (a.start_row == b.start_row && a.start_col <= b.start_col));
	return (Span) {
		.start_row = a.start_row,
		.start_col = a.start_col,
		.end_row = b.end_row,
		.end_col = b.end_col,
	};
}

bool is_ident(char c)
{
	return isalnum(c) || c == '_';
}

void lexer_init(Lexer *lexer, Arena *arena, FILE *file, size_t len)
{
	lexer->len = len;
	lexer->remaining = len;
	lexer->file = file;
	lexer->col = 0;
	lexer->row = 0;
	lexer->arena = arena;

	size_t n = fread(lexer->buf, 1, LEXER_BUF_SIZE, lexer->file);

#ifdef DEBUG_TRACE
	/* TODO: Factor debug trace printf into macro */
	fprintf(stderr, "%s:%d:DEBUG:filled buffer with %lu bytes\n", __FILE__, __LINE__, n);
#endif
}

int lexer_bump(Lexer *lexer)
{
	if (lexer->remaining <= 0) return EOF;
	size_t idx = (lexer->len - lexer->remaining--) % (LEXER_BUF_SIZE);
	int c = lexer->buf[idx];
	if (idx + 1 == (LEXER_BUF_SIZE)) {
		size_t n = fread(lexer->buf, 1, LEXER_BUF_SIZE, lexer->file);

#ifdef DEBUG_TRACE
		/* TODO: Factor debug trace printf into macro */
		fprintf(stderr, "%s:%d:DEBUG:filled buffer with %lu bytes\n", __FILE__, __LINE__, n);
#endif
	}

	lexer->prev_row = lexer->row;
	lexer->prev_col = lexer->col;

	if (c == '\n') {
		++lexer->row;
		lexer->col = 0;
	} else {
		++lexer->col;
	}

	return c;
}

int lexer_peak(Lexer *lexer)
{
	if (lexer->remaining <= 0) return EOF;
	size_t idx = (lexer->len - lexer->remaining) % (LEXER_BUF_SIZE);
	return lexer->buf[idx];
}

#define SBUF_SIZE 16 * 16

void token_name(Token *token, char *buf)
{
	enum TokenKind kind = token->kind;

	if (kind == T_IDENT) {
		sprintf(buf, "IDENT:%s", token->data.s);
		return;
	}

#define TOK(tok) if (kind == tok) { sprintf(buf, #tok); return; }
#include "tokens.h"
#undef TOK
}

void lexer_fill_ident_buf(Lexer *lexer, char **p)
{
	while (is_ident(lexer_peak(lexer))) {
		**p = lexer_bump(lexer);
		++*p;
	}
	**p = '\0';
}

void lexer_consume_ident(Lexer *lexer, Token *token, char c)
{
	char buf[SBUF_SIZE] = {0};
	char *p = buf;
	*p++ = c;

	lexer_fill_ident_buf(lexer, &p);

#define TOK_KW(tok, s)                        \
	if (strncmp(s, buf, BUF_SIZE) == 0) { \
		token->kind = tok;            \
		goto exit;                    \
	}
#include "tokens.h"
#undef TOK_KW

	// Label
	if (lexer_peak(lexer) == ':') {
		lexer_bump(lexer);
		token->kind = T_LABEL;
	} else {
		token->kind = T_IDENT;
	}

	// TODO: Check this out, it wasn't adding null terminator
	// before adding the `1 +`, but I feel like it should be
	// correct without it...
	size_t len = 1 + p - buf;
	char *s = arena_xalloc(lexer->arena, sizeof(*s) * len);
	memcpy(s, buf, sizeof(*s) * len);
	s[len - 1] = '\0';

	token->data.s = s;
exit: ;
}

bool is_num_lit(char c)
{
	return isdigit(c) || c == '.';
}

void lexer_consume_signed_num_lit(Lexer *lexer, Token *token, char c)
{
	bool is_float = false;
	char buf[BUF_SIZE] = {0};
	char peak = lexer_peak(lexer);
	char *p = buf;
	*p++ = c;

	while (is_num_lit(peak)) {
		if (peak == '.') is_float = true;
		*p++ = lexer_bump(lexer);
		peak = lexer_peak(lexer);
	}
	*p = '\0';

	if (is_float) {
		double f = atof(buf);
		token->kind = T_FNUMLIT;
		token->data.f = f;
	} else {
		int i = atoi(buf);
		token->kind = T_INUMLIT;
		token->data.i = i;
	}
}

bool is_valid_digit(char c, int base)
{
	switch (base) {
	case 16:
		return (c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102);
		break;
	default:
		panic("Unknown base; unreachable");
	}
}

bool is_valid_base(char c)
{
	return c == 'x' || c == 'X';
}

void lexer_consume_num_lit(Lexer *lexer, Token *token, char c)
{
	bool is_float = false;
	char buf[BUF_SIZE] = {0};
	char peak = lexer_peak(lexer);
	char *p = buf;
	*p++ = c;

	// Parse different base
	if (c == '0' && isalpha(peak)) {
		int base = 16;
		--p;
		switch (peak) {
		case 'X':
		case 'x':
			base = 16;
			break;
		default:
			// Invalid literal base (0x)
			token->kind = T_ILLEGAL;
			return;
		}

		lexer_bump(lexer);
		peak = lexer_peak(lexer);

		while (is_valid_digit(peak, base)) {
			*p++ = lexer_bump(lexer);
			peak = lexer_peak(lexer);
		}

		long ui = strtol(buf, NULL, base);
		token->kind = T_UINUMLIT;
		token->data.ui = ui;

		return;
	}

	while (is_num_lit(peak)) {
		if (peak == '.') is_float = true;
		*p++ = lexer_bump(lexer);
		peak = lexer_peak(lexer);
	}
	*p = '\0';

	if (is_float) {
		double f = atof(buf);
		token->kind = T_FNUMLIT;
		token->data.f = f;
	} else {
		long ui = atol(buf);
		token->kind = T_UINUMLIT;
		token->data.ui = ui;
	}
}

void lexer_consume_comment(Lexer *lexer)
{
	while (lexer_peak(lexer) != '\n') lexer_bump(lexer);
}

typedef struct StringBuilder {
	size_t len;
	size_t cap;
	char *items;
} StringBuilder;

void string_builder_init(StringBuilder *string_builder, size_t cap)
{
	string_builder->len = 0;
	string_builder->cap = cap;
	string_builder->items = malloc(sizeof(*string_builder->items) * cap);
}

void string_builder_push(StringBuilder *string_builder, char c)
{
	if (string_builder->len >= string_builder->cap) {
		string_builder->cap *= 2;
		string_builder->items = realloc(string_builder->items, string_builder->cap);
	}

	string_builder->items[string_builder->len++] = c;
}

void string_builder_build(StringBuilder *string_builder, char *buf)
{
	memcpy(buf, string_builder->items, string_builder->len);
	buf[string_builder->len] = '\0';
	free(string_builder->items);
}

void lexer_consume_string_lit(Lexer *lexer, Token *token)
{
	StringBuilder string_builder = {0};
	int c = lexer_bump(lexer);
	string_builder_init(&string_builder, 16);
	string_builder_push(&string_builder, c);
	token->kind = T_SLIT;

	while (c != '"') {
		if (c == EOF) {
			// ERROR Unterminated string
			token->kind = T_ILLEGAL;
			return;
		}

		if (c == '\\') {
			c = lexer_bump(lexer);

#define T(from, to) case from: { c = to; break; }
			switch (c) {
			T('0', '\0'); T('a', '\a');
			T('b', '\b'); T('t', '\t');
			T('n', '\n'); T('\\', '\\');
			T('\'', '\'');
			default:
				// ERROR Invalid escape character
				token->kind = T_ILLEGAL;
				return;
			}
#undef T
		}

		string_builder_push(&string_builder, c);
		c = lexer_bump(lexer);
	}

	char *s = arena_xalloc(lexer->arena, string_builder.len + 1);
	string_builder_build(&string_builder, s);
	token->data.s = s;
}

void lexer_consume_char_lit(Lexer *lexer, Token *token)
{
	char c = lexer_bump(lexer);

	if (c == '\\') {
		c = lexer_bump(lexer);

#define T(from, to) case from: { c = to; break; }
		switch (c) {
		T('0', '\0'); T('a', '\a');
		T('b', '\b'); T('t', '\t');
		T('n', '\n'); T('\\', '\\');
		T('\'', '\'');
		default:
			// ERROR Invalid escape character
			token->kind = T_ILLEGAL;
			return;
		}
#undef T
	}

	token->kind = T_INUMLIT;
	token->data.i = c;

	c = lexer_bump(lexer);
	if (c != '\'') {
		// ERROR Too many characters in char lit
		token->kind = T_ILLEGAL;
		return;
	}
}

void lexer_consume_section(Lexer *lexer, Token *token)
{
	char buf[BUF_SIZE] = {0};
	char *p = buf;

	lexer_fill_ident_buf(lexer, &p);

#define CMP_TOK(s, tok) do { if (strncmp(s, buf, BUF_SIZE) == 0) { token->kind = tok; return; } } while(0)
	CMP_TOK("data", T_SECTION_DATA);
	CMP_TOK("text", T_SECTION_TEXT);
#undef CMP_TOK

	// Invalid section name
	token->kind = T_ILLEGAL;
}

Token lexer_next(Lexer *lexer)
{
	Token token = {0};
	size_t start_row;
	size_t start_col;
	char c;
tailcall:
	start_row = lexer->row;
	start_col = lexer->col;
	c = lexer_bump(lexer);

	if (c == EOF) {
		token.kind = T_EOF;
		goto exit;
	}

	if (isalpha(c) || c == '_') {
		lexer_consume_ident(lexer, &token, c);
		goto exit;
	}

	if (c >= '0' && c <= '9') {
		lexer_consume_num_lit(lexer, &token, c);
		goto exit;
	}

	switch (c) {
	case '\t':
	case ' ': goto tailcall;
	case ',': case '[':
	case ']': case '\n':
		token.kind = c;
		break;
	case ';':
		lexer_consume_comment(lexer);
		goto tailcall;
	case '.':
		lexer_consume_section(lexer, &token);
		break;
	case '"':
		lexer_consume_string_lit(lexer, &token);
		break;
	case '\'':
		lexer_consume_char_lit(lexer, &token);
		break;
	case '-':
		lexer_consume_signed_num_lit(lexer, &token, c);
		break;
	default:
		token.kind = T_ILLEGAL;
		goto error;
	}

error:
exit:
	token.span = (Span) {
		.start_row = start_row + 1,
		.start_col = start_col + 1,
		.end_row = lexer->prev_row + 1,
		.end_col = lexer->prev_col + 1,
	};
	return token;
}
