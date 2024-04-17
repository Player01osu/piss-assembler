#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "arena.h"

#define panic(msg) { fprintf(stderr, "%s:%d:"msg, __FILE__, __LINE__); exit(1); }

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

	PRINT_TOK("ILLEGAL", T_ILLEGAL);

	if (kind == T_IDENT) {
		sprintf(buf, "IDENT:%s", token->data.s);
	}
#undef PRINT_TOK
}

void lexer_consume_ident(Lexer *lexer, Token *token, char c)
{
#define CMP_TOK(s, tok) do { if (strncmp(s, buf, BUF_SIZE) == 0) { token->kind = tok; goto exit; } } while(0)
	char buf[BUF_SIZE] = {0};
	char *p = buf;
	*p++ = c;

	while (is_ident(lexer_peak(lexer))) {
		*p++ = lexer_bump(lexer);
	}
	*p = '\0';

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
	char *s = arena_alloc(lexer->arena, sizeof(char) * len);
	memcpy(s, buf, sizeof(char) * len);

	token->data.s = s;
exit: ;
#undef CMP_TOK
}

void lexer_consume_datatype(Lexer *lexer, Token *token)
{
	token->kind = T_DATATYPE;
	char buf[BUF_SIZE] = {0};
	char *p = buf;

	while (is_ident(lexer_peak(lexer))) {
		*p++ = lexer_bump(lexer);
	}
	*p = '\0';

	if (strncmp("int", buf, BUF_SIZE) == 0) {
		token->data.datatype = (Datatype) {
			.kind = TY_PRIMATIVE,
			.data = { .primative = P_INT },
		};
		goto exit;
	}

	size_t len = p - buf;
	char *s = arena_alloc(lexer->arena, sizeof(char) * len);
	memcpy(s, buf, sizeof(char) * len);

	token->data.datatype = (Datatype) {
		.kind = TY_CUSTOM,
		.data = { .s = s },
	};
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
	if (c == '0' && is_valid_base(peak)) {
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

Token *lexer_next(Lexer *lexer)
{
	char c = lexer_bump(lexer);
	Token *token = (Token *)arena_alloc(lexer->arena, sizeof(Token));
tailcall: ;

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
		case '@': {
			lexer_consume_datatype(lexer, token);
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
