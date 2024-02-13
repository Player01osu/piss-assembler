#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

void parser_init(Parser *parser, Arena *arena, const char *src, size_t len)
{
	Lexer lexer = {0};
	lexer_init(&lexer, arena, src, len);
	parser->lexer = lexer;
	parser->arena = arena;
}

Token *parser_bump(Parser *parser)
{
	return lexer_next(&parser->lexer);
}

Token *parser_expect(Parser *parser, enum TokenKind kind)
{
	Token *token = parser_bump(parser);
	if (token->kind == kind) return token;
	else                     return NULL;
}

#define parser_err(parser, msg) { \
	char *_s = (char *) arena_alloc(parser->arena, strlen(msg) + 1); \
	strcpy(_s, msg); \
	parser->error = _s; \
}

#define single_stmt_expect(parser, node_kind)             \
	Token *next = parser_bump(parser);                \
	if (next->kind != T_EOL && next->kind != T_EOF) { \
		node->kind = N_ILLEGAL;                   \
		char __s[128]; \
		sprintf(__s, "%s:Expected eol\n", __func__); \
		parser_err(parser, __s);   \
		goto fail;                                \
	}                                                 \
	node->kind = node_kind;                           \
fail: ;

void parse_push(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	node->kind = N_PUSH;

	if (next->kind == T_NUMLIT) {
		Sized *sized = &node->data.sized;
		sized->size = sizeof(int);
		sized->kind = TY_INT;
		sized->data.i = next->data.i;
	} else if (next->kind == T_IDENT) {
		Sized *sized = &node->data.sized;
		sized->size = sizeof(void *);
		sized->kind = TY_PTR_NAME;
		sized->data.s = next->data.s;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected Literal or Ident");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_pushframe(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_PUSHFRAME);
}

void parse_popframe(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_POPFRAME);
}

void parse_store(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	node->kind = N_STORE;

	if (next->kind == T_NUMLIT) {
		node->data.store.idx = next->data.i;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected size literal");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind != T_COMMA) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Unexpected token; expected ','");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind == T_NUMLIT) {
		Sized *sized = &node->data.store.sized;
		sized->size = sizeof(int);
		sized->kind = TY_INT;
		sized->data.i = next->data.i;
	} else if (next->kind == T_IDENT) {
		Sized *sized = &node->data.store.sized;
		sized->size = sizeof(void *);
		sized->kind = TY_PTR_NAME;
		sized->data.s = next->data.s;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected Literal or Ident");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_storetop(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	node->kind = N_STORETOP;

	if (next->kind == T_NUMLIT) {
		node->data.n = next->data.i;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected size literal");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_load(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	node->kind = N_LOAD;

	if (next->kind == T_NUMLIT) {
		node->data.n = next->data.i;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected size literal");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_pop(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_POP);
}

// Print is really just a debug feature and should be replaced
// w/ something more robust. Or if a print is to be implemented,
// its implementation needs to be much more defined.
void parse_print(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	node->kind = N_PRINT;
	if (next->kind == T_EOL) {
		// TODO: Warn that it is defaulting to printing int
		node->data.ty.size = sizeof(int);
		node->data.ty.kind = TY_INT;
		goto exit;
	}

	if (next->kind == T_DATATYPE) {
		if (next->data.datatype.kind == TY_PRIMATIVE) {
			switch (next->data.datatype.data.primative) {
				case P_INT: {
					node->data.ty.size = sizeof(int);
					node->data.ty.kind = TY_INT;
				} break;
				default: {

				}
			}
		} else {

		}
	}

exit: ;
}

void parse_copy(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	if (next->kind != T_NUMLIT) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected item");
		goto fail;
	}

	node->kind = N_COPY;
	if (next->kind == T_NUMLIT) {
		node->data.n = next->data.i;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected size literal");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_dupe(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_DUPE);
}

void parse_swap(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_SWAP);
}

void parse_add(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_ADD);
}

void parse_sub(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_SUB);
}

void parse_mult(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_MULT);
}

void parse_div(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_DIV);
}

void parse_jump(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	if (next->kind != T_IDENT) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		goto fail;
	}

	node->kind = N_JUMP;
	if (next->kind == T_IDENT) {
		node->data.s = next->data.s;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_jumpcmp(Parser *parser, Node *node)
{
	Token *next = parser_bump(parser);
	if (next->kind != T_IDENT) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		goto fail;
	}

	node->kind = N_JUMPCMP;
	if (next->kind == T_IDENT) {
		node->data.s = next->data.s;
	}

	next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		goto fail;
	}

fail: ;
}

void parse_clt(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_CLT);
}

void parse_cle(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_CLE);
}

void parse_ceq(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_CEQ);
}

void parse_cge(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_CGE);
}

void parse_cgt(Parser *parser, Node *node)
{
	single_stmt_expect(parser, N_CGT);
}

void parse_label(Parser *parser, Token *token, Node *node)
{
	Token *next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		char __s[128];
		sprintf(__s, "%s:Expected eol\n", __func__);
		parser_err(parser, __s);
		goto fail;
	}
	node->kind = N_LABEL;
	node->data.s = token->data.s;
fail: ;
}

Node *parser_next(Parser *parser)
{
redo: ;
	Node *node = arena_alloc(parser->arena, sizeof(Node));
	Token *token = parser_bump(parser);

	switch (token->kind) {
		case T_PUSH: {
			parse_push(parser, node);
		} break;
		case T_POP: {
			parse_pop(parser, node);
		} break;
		case T_PRINT: {
			parse_print(parser, node);
		} break;
		case T_ADD: {
			parse_add(parser, node);
		} break;
		case T_SUB: {
			parse_sub(parser, node);
		} break;
		case T_MULT: {
			parse_mult(parser, node);
		} break;
		case T_DIV: {
			parse_div(parser, node);
		} break;
		case T_JUMP: {
			parse_jump(parser, node);
		} break;
		case T_JUMPCMP: {
			parse_jumpcmp(parser, node);
		} break;
		case T_CLT: {
			parse_clt(parser, node);
		} break;
		case T_CLE: {
			parse_cle(parser, node);
		} break;
		case T_CEQ: {
			parse_ceq(parser, node);
		} break;
		case T_CGE: {
			parse_cge(parser, node);
		} break;
		case T_CGT: {
			parse_cgt(parser, node);
		} break;
		case T_LABEL: {
			parse_label(parser, token, node);
		} break;
		case T_COPY: {
			parse_copy(parser, node);
		} break;
		case T_DUPE: {
			parse_dupe(parser, node);
		} break;
		case T_SWAP: {
			parse_swap(parser, node);
		} break;
		case T_PUSHFRAME: {
			parse_pushframe(parser, node);
		} break;
		case T_POPFRAME: {
			parse_popframe(parser, node);
		} break;
		case T_STORE: {
			parse_store(parser, node);
		} break;
		case T_STORETOP: {
			parse_storetop(parser, node);
		} break;
		case T_LOAD: {
			parse_load(parser, node);
		} break;
		case T_EOF: {
			node->kind = N_EOF;
		} break;
		case T_EOL: {
			goto redo;
		} break;
		default: {
			node->kind = N_ILLEGAL;
			goto error;
		}
	}

error:
	return node;
}
