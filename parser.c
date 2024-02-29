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

#define parser_err(parser, msg) do { \
	size_t len = strlen(msg) + 1; \
	char *_s = (char *) arena_alloc(parser->arena, len); \
	memcpy(_s, msg, len); \
	parser->error = _s; \
} while(0)

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

void parse_push(Parser *parser, Node *node, enum NodeKind kind)
{
	Token *next = parser_bump(parser);
	Lit *lit = &node->data.lit;
	node->kind = kind;

	if (next->kind == T_INUMLIT) {
		lit->kind = L_INT;
		lit->data.i = next->data.i;
	} else if (next->kind == T_UINUMLIT) {
		lit->kind = L_UINT;
		lit->data.ui = next->data.ui;
	} else if (next->kind == T_FNUMLIT) {
		lit->kind = L_FLOAT;
		lit->data.f = next->data.f;
	} else if (next->kind == T_IDENT) {
		lit->kind = L_PTR;
		lit->data.s = next->data.s;
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

void parse_idx(Parser *parser, Node *node, enum NodeKind kind)
{
	Token *next = parser_bump(parser);
	node->kind = kind;

	if (next->kind == T_UINUMLIT) {
		node->data.n = next->data.ui;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected index");
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

void parse_single_stmt(Parser *parser, Node *node, enum NodeKind kind)
{
	single_stmt_expect(parser, kind);
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

void parse_jumpcmp(Parser *parser, Node *node, enum NodeKind kind)
{
	Token *next = parser_bump(parser);
	if (next->kind != T_IDENT) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		goto fail;
	}

	node->kind = kind;
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

void parse_jumpproc(Parser *parser, Node *node)
{
	node->kind = N_JUMPPROC;

	Token *next = parser_bump(parser);
	if (next->kind == T_IDENT) {
		node->data.proc.location.s = next->data.s;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		goto fail;
	}

	next = parser_bump(parser);
	if (next->kind == T_UINUMLIT) {
		node->data.proc.argc = next->data.ui;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected number of bytes");
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

void parse_label(Parser *parser, Token *token, Node *node)
{
	Token *next = parser_bump(parser);
	if (next->kind != T_EOL && next->kind != T_EOF) {
		node->kind = N_ILLEGAL;
		char __s[256];
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
		case T_IPUSH: {
			parse_push(parser, node, N_IPUSH);
		} break;
		case T_IADD: {
			parse_single_stmt(parser, node, N_IADD);
		} break;
		case T_ISUB: {
			parse_single_stmt(parser, node, N_ISUB);
		} break;
		case T_IMULT: {
			parse_single_stmt(parser, node, N_IMULT);
		} break;
		case T_IDIV: {
			parse_single_stmt(parser, node, N_IDIV);
		} break;
		case T_IMOD: {
			parse_single_stmt(parser, node, N_IMOD);
		} break;
		case T_IPRINT: {
			parse_single_stmt(parser, node, N_IPRINT);
		} break;

		case T_FPUSH: {
			parse_push(parser, node, N_FPUSH);
		} break;
		case T_FADD: {
			parse_single_stmt(parser, node, N_FADD);
		} break;
		case T_FSUB: {
			parse_single_stmt(parser, node, N_FSUB);
		} break;
		case T_FMULT: {
			parse_single_stmt(parser, node, N_FMULT);
		} break;
		case T_FDIV: {
			parse_single_stmt(parser, node, N_FDIV);
		} break;
		case T_FPRINT: {
			parse_single_stmt(parser, node, N_FPRINT);
		} break;

		case T_CPUSH: {
			parse_push(parser, node, N_CPUSH);
		} break;
		case T_CADD: {
			parse_single_stmt(parser, node, N_CADD);
		} break;
		case T_CSUB: {
			parse_single_stmt(parser, node, N_CSUB);
		} break;
		case T_CMULT: {
			parse_single_stmt(parser, node, N_CMULT);
		} break;
		case T_CDIV: {
			parse_single_stmt(parser, node, N_CDIV);
		} break;
		case T_CMOD: {
			parse_single_stmt(parser, node, N_CMOD);
		} break;
		case T_CPRINT: {
			parse_single_stmt(parser, node, N_CPRINT);
		} break;
		case T_CIPRINT: {
			parse_single_stmt(parser, node, N_CIPRINT);
		} break;

		case T_POP8: {
			parse_single_stmt(parser, node, N_POP8);
		} break;
		case T_POP32: {
			parse_single_stmt(parser, node, N_POP32);
		} break;
		case T_POP64: {
			parse_single_stmt(parser, node, N_POP64);
		} break;

		case T_JUMP: {
			parse_jump(parser, node);
		} break;
		case T_JUMPCMP: {
			parse_jumpcmp(parser, node, N_JUMPCMP);
		} break;
		case T_JUMPPROC: {
			parse_jumpproc(parser, node);
		} break;

		case T_ICLT: {
			parse_single_stmt(parser, node, N_ICLT);
		} break;
		case T_ICLE: {
			parse_single_stmt(parser, node, N_ICLE);
		} break;
		case T_ICEQ: {
			parse_single_stmt(parser, node, N_ICEQ);
		} break;
		case T_ICGT: {
			parse_single_stmt(parser, node, N_ICGT);
		} break;
		case T_ICGE: {
			parse_single_stmt(parser, node, N_ICGE);
		} break;

		case T_FCLT: {
			parse_single_stmt(parser, node, N_FCLT);
		} break;
		case T_FCLE: {
			parse_single_stmt(parser, node, N_FCLE);
		} break;
		case T_FCEQ: {
			parse_single_stmt(parser, node, N_FCEQ);
		} break;
		case T_FCGT: {
			parse_single_stmt(parser, node, N_FCGT);
		} break;
		case T_FCGE: {
			parse_single_stmt(parser, node, N_FCGE);
		} break;

		case T_CCLT: {
			parse_single_stmt(parser, node, N_CCLT);
		} break;
		case T_CCLE: {
			parse_single_stmt(parser, node, N_CCLE);
		} break;
		case T_CCEQ: {
			parse_single_stmt(parser, node, N_CCEQ);
		} break;
		case T_CCGT: {
			parse_single_stmt(parser, node, N_CCGT);
		} break;
		case T_CCGE: {
			parse_single_stmt(parser, node, N_CCGE);
		} break;

		case T_LABEL: {
			parse_label(parser, token, node);
		} break;

		case T_DUPE8: {
			parse_single_stmt(parser, node, N_DUPE8);
		} break;
		case T_DUPE32: {
			parse_single_stmt(parser, node, N_DUPE32);
		} break;
		case T_DUPE64: {
			parse_single_stmt(parser, node, N_DUPE64);
		} break;

		case T_SWAP8: {
			parse_single_stmt(parser, node, N_SWAP8);
		} break;
		case T_SWAP32: {
			parse_single_stmt(parser, node, N_SWAP32);
		} break;
		case T_SWAP64: {
			parse_single_stmt(parser, node, N_SWAP64);
		} break;

		case T_COPY8: {
			parse_idx(parser, node, N_COPY8);
		} break;
		case T_COPY32: {
			parse_idx(parser, node, N_COPY32);
		} break;
		case T_COPY64: {
			parse_idx(parser, node, N_COPY64);
		} break;

		case T_STORE8: {
			parse_idx(parser, node, N_STORE8);
		} break;
		case T_STORE32: {
			parse_idx(parser, node, N_STORE32);
		} break;
		case T_STORE64: {
			parse_idx(parser, node, N_STORE64);
		} break;

		case T_LOAD8: {
			parse_idx(parser, node, N_LOAD8);
		} break;
		case T_LOAD32: {
			parse_idx(parser, node, N_LOAD32);
		} break;
		case T_LOAD64: {
			parse_idx(parser, node, N_LOAD64);
		} break;

		case T_RET8: {
			parse_single_stmt(parser, node, N_RET8);
		} break;
		case T_RET32: {
			parse_single_stmt(parser, node, N_RET32);
		} break;
		case T_RET64: {
			parse_single_stmt(parser, node, N_RET64);
		} break;

		case T_RET: {
			parse_idx(parser, node, N_RET);
		} break;

		case T_EOF: {
			node->kind = N_EOF;
		} break;
		case T_EOL: {
			goto redo;
		} break;
		default: {
			node->kind = N_ILLEGAL;
			char msg[256] = {0};
			char name[256] = {0};
			token_name(token, name);
			sprintf(msg, "Unexpected start of statement:%s", name);
			parser_err(parser, msg);
			goto error;
		}
	}

error:
	return node;
}
