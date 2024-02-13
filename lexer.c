#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "arena.h"

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

void lexer_consume_ident(Lexer *lexer, Token *token, char c)
{
	char buf[BUF_SIZE] = {0};
	char *p = buf;
	*p++ = c;

	while (is_ident(lexer_peak(lexer))) {
		*p++ = lexer_bump(lexer);
	}
	*p = '\0';

	if (strncmp("push", buf, BUF_SIZE) == 0) {
		token->kind = T_PUSH;
		goto exit;
	} else if (strncmp("pop", buf, BUF_SIZE) == 0) {
		token->kind = T_POP;
		goto exit;
	} else if (strncmp("print", buf, BUF_SIZE) == 0) {
		token->kind = T_PRINT;
		goto exit;
	} else if (strncmp("add", buf, BUF_SIZE) == 0) {
		token->kind = T_ADD;
		goto exit;
	} else if (strncmp("sub", buf, BUF_SIZE) == 0) {
		token->kind = T_SUB;
		goto exit;
	} else if (strncmp("mult", buf, BUF_SIZE) == 0) {
		token->kind = T_MULT;
		goto exit;
	} else if (strncmp("div", buf, BUF_SIZE) == 0) {
		token->kind = T_DIV;
		goto exit;
	} else if (strncmp("jump", buf, BUF_SIZE) == 0) {
		token->kind = T_JUMP;
		goto exit;
	} else if (strncmp("jumpcmp", buf, BUF_SIZE) == 0) {
		token->kind = T_JUMPCMP;
		goto exit;
	} else if (strncmp("clt", buf, BUF_SIZE) == 0) {
		token->kind = T_CLT;
		goto exit;
	} else if (strncmp("cle", buf, BUF_SIZE) == 0) {
		token->kind = T_CLE;
		goto exit;
	} else if (strncmp("ceq", buf, BUF_SIZE) == 0) {
		token->kind = T_CEQ;
		goto exit;
	} else if (strncmp("cge", buf, BUF_SIZE) == 0) {
		token->kind = T_CGE;
		goto exit;
	} else if (strncmp("cgt", buf, BUF_SIZE) == 0) {
		token->kind = T_CGT;
		goto exit;
	} else if (strncmp("copy", buf, BUF_SIZE) == 0) {
		token->kind = T_COPY;
		goto exit;
	} else if (strncmp("dupe", buf, BUF_SIZE) == 0) {
		token->kind = T_DUPE;
		goto exit;
	} else if (strncmp("swap", buf, BUF_SIZE) == 0) {
		token->kind = T_SWAP;
		goto exit;
	} else if (strncmp("load", buf, BUF_SIZE) == 0) {
		token->kind = T_LOAD;
		goto exit;
	} else if (strncmp("store", buf, BUF_SIZE) == 0) {
		token->kind = T_STORE;
		goto exit;
	} else if (strncmp("storetop", buf, BUF_SIZE) == 0) {
		token->kind = T_STORETOP;
		goto exit;
	} else if (strncmp("popframe", buf, BUF_SIZE) == 0) {
		token->kind = T_POPFRAME;
		goto exit;
	} else if (strncmp("pushframe", buf, BUF_SIZE) == 0) {
		token->kind = T_PUSHFRAME;
		goto exit;
	} else {
		// Label
		if (lexer_peak(lexer) == ':') {
			lexer_bump(lexer);
			token->kind = T_LABEL;
		} else {
			token->kind = T_IDENT;
		}
	}

	// TODO: Check this out, it wasn't adding null terminator
	// before adding the `1 +`, but I feel like it should be
	// correct without it...
	size_t len = 1 + p - buf;
	char *s = arena_alloc(lexer->arena, sizeof(char) * len);
	memcpy(s, buf, sizeof(char) * len);

	token->data.s = s;
exit: ;
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

void lexer_consume_num_lit(Lexer *lexer, Token *token, char c)
{
	char buf[BUF_SIZE] = {0};
	char *p = buf;
	*p++ = c;

	while (isdigit(lexer_peak(lexer))) {
		*p++ = lexer_bump(lexer);
	}
	*p = '\0';

	int i = atoi(buf);
	token->kind = T_NUMLIT;
	token->data.i = i;
}

void lexer_consume_comment(Lexer *lexer)
{
	while (lexer_peak(lexer) != '\n') lexer_bump(lexer);
}

Token *lexer_next(Lexer *lexer)
{
redo: ;
	char c = lexer_bump(lexer);
	Token *token = (Token *)arena_alloc(lexer->arena, sizeof(Token));

	if (c == '\0') {
		token->kind = T_EOF;
		goto exit;
	}

	if (isalpha(c)) {
		lexer_consume_ident(lexer, token, c);
		goto exit;
	}

	if (c >= '0' && c <= '9') {
		lexer_consume_num_lit(lexer, token, c);
		goto exit;
	}

	switch (c) {
		case '\t':
		case ' ': goto redo;
		case '\n': {
			token->kind = T_EOL;
		} break;
		case ';': {
			lexer_consume_comment(lexer);
			goto redo;
		} break;
		case ',': {
			token->kind = T_COMMA;
		} break;
		case '@': {
			lexer_consume_datatype(lexer, token);
		} break;
		default: {

			token->kind = T_ILLEGAL;
			goto error;
		} break;
	}

error:
exit:
	return token;
}
