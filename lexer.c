#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "arena.h"

#define panic(msg) { fprintf(stderr, "%s:%d:"msg, __FILE__, __LINE__); exit(1); }

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

void lexer_init(Lexer *lexer, Arena *arena, const char *src, size_t len)
{
	lexer->len = len;
	lexer->src = src;
	lexer->cursor = src;
	lexer->remaining = len;
	lexer->col = 0;
	lexer->row = 0;
	lexer->arena = arena;
}

char lexer_bump(Lexer *lexer)
{
	if (lexer->remaining <= 0) return '\0';

	lexer->prev_row = lexer->row;
	lexer->prev_col = lexer->col;

	if (*lexer->cursor == '\n') {
		++lexer->row;
		lexer->col = 0;
	} else {
		++lexer->col;
	}

	--lexer->remaining;
	return *lexer->cursor++;
}

char lexer_peak(Lexer *lexer)
{
	if (lexer->remaining <= 0) return '\0';
	return *lexer->cursor;
}

#define BUF_SIZE 16 * 16

void token_name(Token *token, char *buf)
{
#define PRINT_TOK(s, k) do { \
	if (kind == k) { sprintf(buf, "%s", s); } \
	} while(0)
	enum TokenKind kind = token->kind;

	PRINT_TOK("pop8", T_POP8);
	PRINT_TOK("pop32", T_POP32);
	PRINT_TOK("pop64", T_POP64);

	PRINT_TOK("dupe8", T_DUPE8);
	PRINT_TOK("dupe32", T_DUPE32);
	PRINT_TOK("dupe64", T_DUPE64);

	PRINT_TOK("swap8", T_SWAP8);
	PRINT_TOK("swap32", T_SWAP32);
	PRINT_TOK("swap64", T_SWAP64);

	PRINT_TOK("copy8", T_COPY8);
	PRINT_TOK("copy32", T_COPY32);
	PRINT_TOK("copy64", T_COPY64);

	PRINT_TOK("store8", T_STORE8);
	PRINT_TOK("store32", T_STORE32);
	PRINT_TOK("store64", T_STORE64);

	PRINT_TOK("load8", T_LOAD8);
	PRINT_TOK("load32", T_LOAD32);
	PRINT_TOK("load64", T_LOAD64);

	PRINT_TOK("ret8", T_RET8);
	PRINT_TOK("ret32", T_RET32);
	PRINT_TOK("ret64", T_RET64);

	PRINT_TOK("ret", T_RET);

	PRINT_TOK("jump", T_JUMP);
	PRINT_TOK("jumpcmp", T_JUMPCMP);
	PRINT_TOK("jumpproc", T_JUMPPROC);

	PRINT_TOK("iclt", T_ICLT);
	PRINT_TOK("icle", T_ICLE);
	PRINT_TOK("iceq", T_ICEQ);
	PRINT_TOK("icgt", T_ICGT);
	PRINT_TOK("icge", T_ICGE);

	PRINT_TOK("ulclt", T_ULCLT);
	PRINT_TOK("ulcle", T_ULCLE);
	PRINT_TOK("ulceq", T_ULCEQ);
	PRINT_TOK("ulcgt", T_ULCGT);
	PRINT_TOK("ulcge", T_ULCGE);

	PRINT_TOK("fclt", T_FCLT);
	PRINT_TOK("fcle", T_FCLE);
	PRINT_TOK("fceq", T_FCEQ);
	PRINT_TOK("fcgt", T_FCGT);
	PRINT_TOK("fcge", T_FCGE);

	PRINT_TOK("cclt", T_CCLT);
	PRINT_TOK("ccle", T_CCLE);
	PRINT_TOK("cceq", T_CCEQ);
	PRINT_TOK("ccgt", T_CCGT);
	PRINT_TOK("ccge", T_CCGE);

	PRINT_TOK("ulpush", T_ULPUSH);
	PRINT_TOK("uladd", T_ULADD);
	PRINT_TOK("ulsub", T_ULSUB);
	PRINT_TOK("ulmult", T_ULMULT);
	PRINT_TOK("uldiv", T_ULDIV);
	PRINT_TOK("ulmod", T_ULMOD);
	PRINT_TOK("ulprint", T_ULPRINT);

	PRINT_TOK("ipush", T_IPUSH);
	PRINT_TOK("iadd", T_IADD);
	PRINT_TOK("isub", T_ISUB);
	PRINT_TOK("imult", T_IMULT);
	PRINT_TOK("idiv", T_IDIV);
	PRINT_TOK("imod", T_IMOD);
	PRINT_TOK("iprint", T_IPRINT);

	PRINT_TOK("fpush", T_FPUSH);
	PRINT_TOK("fadd", T_FADD);
	PRINT_TOK("fsub", T_FSUB);
	PRINT_TOK("fmult", T_FMULT);
	PRINT_TOK("fdiv", T_FDIV);
	PRINT_TOK("fprint", T_FPRINT);

	PRINT_TOK("cpush", T_CPUSH);
	PRINT_TOK("cadd", T_CADD);
	PRINT_TOK("csub", T_CSUB);
	PRINT_TOK("cmult", T_CMULT);
	PRINT_TOK("cdiv", T_CDIV);
	PRINT_TOK("cmod", T_CMOD);
	PRINT_TOK("cprint", T_CPRINT);
	PRINT_TOK("ciprint", T_CIPRINT);

	PRINT_TOK("ppush", T_PPUSH);
	PRINT_TOK("pload", T_PLOAD);

	PRINT_TOK(".data", T_SECTION_DATA);
	PRINT_TOK(".text", T_SECTION_TEXT);

	PRINT_TOK("ILLEGAL", T_ILLEGAL);

	if (kind == T_IDENT) {
		sprintf(buf, "IDENT:%s", token->data.s);
	}
#undef PRINT_TOK
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
#define CMP_TOK(s, tok) do { if (strncmp(s, buf, BUF_SIZE) == 0) { token->kind = tok; goto exit; } } while(0)
	char buf[BUF_SIZE] = {0};
	char *p = buf;
	*p++ = c;

	lexer_fill_ident_buf(lexer, &p);

	CMP_TOK("pop8", T_POP8);
	CMP_TOK("pop32", T_POP32);
	CMP_TOK("pop64", T_POP64);

	CMP_TOK("dupe8", T_DUPE8);
	CMP_TOK("dupe32", T_DUPE32);
	CMP_TOK("dupe64", T_DUPE64);

	CMP_TOK("swap8", T_SWAP8);
	CMP_TOK("swap32", T_SWAP32);
	CMP_TOK("swap64", T_SWAP64);

	CMP_TOK("copy8", T_COPY8);
	CMP_TOK("copy32", T_COPY32);
	CMP_TOK("copy64", T_COPY64);

	CMP_TOK("store8", T_STORE8);
	CMP_TOK("store32", T_STORE32);
	CMP_TOK("store64", T_STORE64);

	CMP_TOK("load8", T_LOAD8);
	CMP_TOK("load32", T_LOAD32);
	CMP_TOK("load64", T_LOAD64);

	CMP_TOK("ret8", T_RET8);
	CMP_TOK("ret32", T_RET32);
	CMP_TOK("ret64", T_RET64);

	CMP_TOK("ret", T_RET);

	CMP_TOK("jump", T_JUMP);
	CMP_TOK("jumpcmp", T_JUMPCMP);
	CMP_TOK("jumpproc", T_JUMPPROC);

	CMP_TOK("iclt", T_ICLT);
	CMP_TOK("icle", T_ICLE);
	CMP_TOK("iceq", T_ICEQ);
	CMP_TOK("icgt", T_ICGT);
	CMP_TOK("icge", T_ICGE);

	CMP_TOK("ulclt", T_ULCLT);
	CMP_TOK("ulcle", T_ULCLE);
	CMP_TOK("ulceq", T_ULCEQ);
	CMP_TOK("ulcgt", T_ULCGT);
	CMP_TOK("ulcge", T_ULCGE);

	CMP_TOK("fclt", T_FCLT);
	CMP_TOK("fcle", T_FCLE);
	CMP_TOK("fceq", T_FCEQ);
	CMP_TOK("fcgt", T_FCGT);
	CMP_TOK("fcge", T_FCGE);

	CMP_TOK("cclt", T_CCLT);
	CMP_TOK("ccle", T_CCLE);
	CMP_TOK("cceq", T_CCEQ);
	CMP_TOK("ccgt", T_CCGT);
	CMP_TOK("ccge", T_CCGE);

	CMP_TOK("ulpush", T_ULPUSH);
	CMP_TOK("uladd", T_ULADD);
	CMP_TOK("ulsub", T_ULSUB);
	CMP_TOK("ulmult", T_ULMULT);
	CMP_TOK("uldiv", T_ULDIV);
	CMP_TOK("ulmod", T_ULMOD);
	CMP_TOK("ulprint", T_ULPRINT);

	CMP_TOK("ipush", T_IPUSH);
	CMP_TOK("iadd", T_IADD);
	CMP_TOK("isub", T_ISUB);
	CMP_TOK("imult", T_IMULT);
	CMP_TOK("idiv", T_IDIV);
	CMP_TOK("imod", T_IMOD);
	CMP_TOK("iprint", T_IPRINT);

	CMP_TOK("fpush", T_FPUSH);
	CMP_TOK("fadd", T_FADD);
	CMP_TOK("fsub", T_FSUB);
	CMP_TOK("fmult", T_FMULT);
	CMP_TOK("fdiv", T_FDIV);
	CMP_TOK("fprint", T_FPRINT);

	CMP_TOK("cpush", T_CPUSH);
	CMP_TOK("cadd", T_CADD);
	CMP_TOK("csub", T_CSUB);
	CMP_TOK("cmult", T_CMULT);
	CMP_TOK("cdiv", T_CDIV);
	CMP_TOK("cmod", T_CMOD);
	CMP_TOK("cprint", T_CPRINT);
	CMP_TOK("ciprint", T_CIPRINT);

	CMP_TOK("ppush",   T_PPUSH);
	CMP_TOK("pload",   T_PLOAD);
	CMP_TOK("pderef8", T_PDEREF8);
	CMP_TOK("pderef32",T_PDEREF32);
	CMP_TOK("pderef64",T_PDEREF64);
	CMP_TOK("pderef",  T_PDEREF);
	CMP_TOK("pset8",   T_PSET8);
	CMP_TOK("pset32",  T_PSET32);
	CMP_TOK("pset64",  T_PSET64);
	CMP_TOK("pset",    T_PSET);

	CMP_TOK("extern", T_EXTERN);

	CMP_TOK("dd", T_DD);
	CMP_TOK("dw", T_DW);
	CMP_TOK("db", T_DB);

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
	char *s = arena_alloc(lexer->arena, sizeof(*s) * len);
	memcpy(s, buf, sizeof(*s) * len);
	s[len - 1] = '\0';

	token->data.s = s;
exit: ;
#undef CMP_TOK
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
		case 16: {
			return (c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102);
		} break;
		default: {
			panic("Unknown base; unreachable");
		}
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
			case 'x': {
				base = 16;
			} break;
			default: {
				// Invalid literal base (0x)
				token->kind = T_ILLEGAL;
				goto error;
			}
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
error: ;
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
	char c = lexer_bump(lexer);
	string_builder_init(&string_builder, 16);
	string_builder_push(&string_builder, c);
	token->kind = T_SLIT;

	while (c != '"') {
		if (c == '\0') {
			// ERROR Unterminated string
			token->kind = T_ILLEGAL;
			goto error;
		}

		if (c == '\\') {
			c = lexer_bump(lexer);

#define T(from, to) case from: { c = to; break; }
			switch (c) {
				T('0', '\0'); T('a', '\a');
				T('b', '\b'); T('t', '\t');
				T('n', '\n'); T('\\', '\\');
				T('\'', '\'');
				default: {
					// ERROR Invalid escape character
					token->kind = T_ILLEGAL;
					goto error;
				}
			}
#undef T
		}

		string_builder_push(&string_builder, c);
		c = lexer_bump(lexer);
	}

	char *s = arena_alloc(lexer->arena, string_builder.len + 1);
	string_builder_build(&string_builder, s);
	token->data.s = s;
error: ;
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
			default: {
				// ERROR Invalid escape character
				token->kind = T_ILLEGAL;
				goto error;
			}
		}
#undef T
	}

	token->kind = T_INUMLIT;
	token->data.i = c;

	c = lexer_bump(lexer);
	if (c != '\'') {
		// ERROR Too many characters in char lit
		token->kind = T_ILLEGAL;
		goto error;
	}
error: ;
}

void lexer_consume_section(Lexer *lexer, Token *token)
{
#define CMP_TOK(s, tok) do { if (strncmp(s, buf, BUF_SIZE) == 0) { token->kind = tok; goto exit; } } while(0)
	char buf[BUF_SIZE] = {0};
	char *p = buf;

	lexer_fill_ident_buf(lexer, &p);

	CMP_TOK("data", T_SECTION_DATA);
	CMP_TOK("text", T_SECTION_TEXT);

	// Invalid section name
	token->kind = T_ILLEGAL;
exit: ;

#undef CMP_TOK
}

Token *lexer_next(Lexer *lexer)
{
	Token *token = (Token *)arena_alloc(lexer->arena, sizeof(Token));
tailcall: ;
	char c = lexer_bump(lexer);
	size_t start_row = lexer->row;
	size_t start_col = lexer->col;

	if (c == '\0') {
		token->kind = T_EOF;
		goto exit;
	}

	if (isalpha(c) || c == '_') {
		lexer_consume_ident(lexer, token, c);
		goto exit;
	}

	if (c >= '0' && c <= '9') {
		lexer_consume_num_lit(lexer, token, c);
		goto exit;
	}

	switch (c) {
		case '\t':
		case ' ': goto tailcall;
		case '\n': {
			token->kind = T_EOL;
		} break;
		case ';': {
			lexer_consume_comment(lexer);
			goto tailcall;
		} break;
		case ',': {
			token->kind = T_COMMA;
		} break;
		case '[': {
			token->kind = T_OPEN_BRACKET;
		} break;
		case ']': {
			token->kind = T_CLOSE_BRACKET;
		} break;
		case '.': {
			lexer_consume_section(lexer, token);
		} break;

		case '"': {
			lexer_consume_string_lit(lexer, token);
		} break;
		case '\'': {
			lexer_consume_char_lit(lexer, token);
		} break;

		case '-': {
			lexer_consume_signed_num_lit(lexer, token, c);
		} break;
		default: {

			token->kind = T_ILLEGAL;
			goto error;
		} break;
	}

error:
exit:
	token->span = (Span) {
		.start_row = start_row + 1,
		.start_col = start_col + 1,
		.end_row = lexer->prev_row + 1,
		.end_col = lexer->prev_col + 1,
	};
	return token;
}
