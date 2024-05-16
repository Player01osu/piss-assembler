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
	parser->state = PARSE_TEXT;
}

bool is_end_of_statement(enum TokenKind kind)
{
	return kind == T_EOL || kind == T_EOF;
}

Token parser_bump(Parser *parser)
{
	return lexer_next(&parser->lexer);
}

int parser_expect(Parser *parser, enum TokenKind kind, Token *token)
{
	*token = parser_bump(parser);
	if (token->kind == kind) return 0;
	else                     return -1;
}

#define parser_err(parser, msg)                                      \
	do {                                                         \
		size_t len = strlen(msg) + 1;                        \
		char *_s = (char *) arena_alloc(parser->arena, len); \
		memcpy(_s, msg, len);                                \
		parser->error = _s;                                  \
	} while (0)

#define single_stmt_expect(parser, instruction_kind)                 \
	do {                                                         \
		Token next = parser_bump(parser);                    \
		if (next.kind != T_EOL && next.kind != T_EOF) {      \
			node->kind = N_ILLEGAL;                      \
			char __s[128];                               \
			sprintf(__s, "%s:Expected eol\n", __func__); \
			parser_err(parser, __s);                     \
			return -1;                                   \
		}                                                    \
		node->data.instruction.kind = instruction_kind;      \
		node->kind = N_INSTRUCTION;                          \
		node->span = span_join(node->span, next.span);       \
		return 0;                                            \
	} while (0)

int parse_push(Parser *parser, Node *node, enum InstructionKind kind)
{
	Token next = parser_bump(parser);
	Lit *lit = &node->data.instruction.data.lit;
	node->span = span_join(node->span, next.span);
	node->kind = N_INSTRUCTION;
	node->data.instruction.kind = kind;

	if (next.kind == T_INUMLIT) {
		lit->kind = L_INT;
		lit->data.i = next.data.i;
	} else if (next.kind == T_UINUMLIT) {
		lit->kind = L_UINT;
		lit->data.ui = next.data.ui;
	} else if (next.kind == T_FNUMLIT) {
		lit->kind = L_FLOAT;
		lit->data.f = next.data.f;
	} else if (next.kind == T_IDENT) {
		lit->kind = L_PTR;
		lit->data.s = next.data.s;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected Literal or Ident");
		return -1;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		return -1;
	}

	return 0;
}

int parse_idx(Parser *parser, Node *node, enum InstructionKind kind)
{
	Token next = parser_bump(parser);
	node->span = span_join(node->span, next.span);
	node->data.instruction.kind = kind;
	node->kind = N_INSTRUCTION;
	node->span = next.span;

	if (next.kind == T_UINUMLIT) {
		node->data.instruction.data.n = next.data.ui;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected index");
		return -1;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		return -1;
	}

	return 0;
}

int parse_single_stmt(Parser *parser, Node *node, enum InstructionKind kind)
{
	single_stmt_expect(parser, kind);
}

int parse_jump(Parser *parser, Node *node)
{
	Token next = parser_bump(parser);
	node->span = span_join(node->span, next.span);
	if (next.kind != T_IDENT) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		return -1;
	}

	node->data.instruction.kind = I_JUMP;
	node->kind = N_INSTRUCTION;

	if (next.kind == T_IDENT) {
		node->data.instruction.data.s = next.data.s;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		return -1;
	}

	return 0;
}

int parse_jumpcmp(Parser *parser, Node *node, enum InstructionKind kind)
{
	Token next = parser_bump(parser);
	node->span = span_join(node->span, next.span);
	if (next.kind != T_IDENT) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		return -1;
	}

	node->data.instruction.kind = kind;
	node->kind = N_INSTRUCTION;

	if (next.kind == T_IDENT) {
		node->data.instruction.data.s = next.data.s;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		return -1;
	}

	return 0;
}

int parse_jumpproc(Parser *parser, Node *node)
{
	Token next = parser_bump(parser);
	node->data.instruction.kind = I_JUMPPROC;
	node->kind = N_INSTRUCTION;
	node->span = span_join(node->span, next.span);

	if (next.kind == T_IDENT) {
		node->data.instruction.data.proc.location.s = next.data.s;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected label");
		return -1;
	}

	next = parser_bump(parser);
	node->span = span_join(node->span, next.span);
	if (next.kind == T_UINUMLIT) {
		node->data.instruction.data.proc.argc = next.data.ui;
	} else {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected number of bytes");
		return -1;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
		node->kind = N_ILLEGAL;
		parser_err(parser, "Expected newline");
		return -1;
	}
	return 0;
}

int parse_label(Parser *parser, Token *token, Node *node)
{
	Token next = parser_bump(parser);
	node->span = span_join(node->span, token->span);
	if (!is_end_of_statement(next.kind)) {
		node->kind = N_ILLEGAL;
		char __s[256];
		sprintf(__s, "%s:Expected end of statement\n", __func__);
		parser_err(parser, __s);
		return -1;
	}
	node->kind = N_LABEL;
	node->data.s = token->data.s;
	return 0;
}

int parse_data(Parser *parser, Token *token, Node *node)
{
	Token next = parser_bump(parser);
	node->span = span_join(node->span, next.span);
	node->data.declaration.ident = token->data.s;
	node->kind = N_DECLARATION;

	if (next.kind == T_DD || next.kind == T_DB || next.kind == T_DW) {
		// Consume expecting ui and i numlit and slit
		// until end of statement
		size_t ty_size = 0;
		switch (next.kind) {
		case T_DD: {
			node->data.declaration.kind = D_DD;
			ty_size = 8;
		} break;
		case T_DB: {
			node->data.declaration.kind = D_DB;
			ty_size = 1;
		} break;
		case T_DW: {
			node->data.declaration.kind = D_DW;
			ty_size = 4;
		} break;
		default: {
			panic("Unreachable\n");
		}
		}

		next = parser_bump(parser);
		if (next.kind == T_OPEN_BRACKET) {
			next = parser_bump(parser);
			if (next.kind != T_UINUMLIT) {
				node->kind = N_ILLEGAL;
				parser_err(parser, "expected an unsigned integer");
				return -1;
			}

			node->data.declaration.bytes = calloc(next.data.ui, ty_size);
			node->data.declaration.len = next.data.ui * ty_size;
			node->span = span_join(node->span, next.span);
			node->data.declaration.span = node->span;

			next = parser_bump(parser);
			if (next.kind != T_CLOSE_BRACKET) {
				node->kind = N_ILLEGAL;
				parser_err(parser, "unclosed bracket");
				return -1;
			}
		} else {
			panic("Unimplemented\n");
		}
	} else if (next.kind == T_EXTERN) {
		next = parser_bump(parser);
		node->data.declaration.kind = D_EXTERN;

		if (!is_end_of_statement(next.kind)) {
			node->kind = N_ILLEGAL;
			char __s[256];
			sprintf(__s, "%s:Expected end of statement\n", __func__);
			parser_err(parser, __s);
			return -1;
		}
	} else {
		node->kind = N_ILLEGAL;
		char __s[256];
		sprintf(__s, "%s:Expected data kind\n", __func__);
		parser_err(parser, __s);
		return -1;
	}

	return 0;
}

Node *parser_next(Parser *parser)
{
tailcall: ;
	Node *node = arena_alloc(parser->arena, sizeof(Node));
	Token token = parser_bump(parser);

	node->span = token.span;
	switch (token.kind) {
	case T_SECTION_TEXT: {
		parser->state = PARSE_TEXT;
		goto tailcall;
	} break;
	case T_SECTION_DATA: {
		parser->state = PARSE_DATA;
		goto tailcall;
	} break;
	case T_EOL: {
		goto tailcall;
	} break;
	default: ;
	}

	switch (parser->state) {
	case PARSE_DATA: {
		switch (token.kind) {
		case T_IDENT: {
			parse_data(parser, &token, node);
		} break;
		default: {
			node->kind = N_ILLEGAL;
			char msg[256] = {0};
			char name[256] = {0};
			token_name(&token, name);
			sprintf(msg, "Unexpected start of statement:%s", name);
			parser_err(parser, msg);
			goto error;
		}
		}
	} break;
	case PARSE_TEXT: {
		switch (token.kind) {
		case T_ULPUSH: {
			parse_push(parser, node, I_ULPUSH);
		} break;
		case T_ULADD: {
			parse_single_stmt(parser, node, I_ULADD);
		} break;
		case T_ULSUB: {
			parse_single_stmt(parser, node, I_ULSUB);
		} break;
		case T_ULMULT: {
			parse_single_stmt(parser, node, I_ULMULT);
		} break;
		case T_ULDIV: {
			parse_single_stmt(parser, node, I_ULDIV);
		} break;
		case T_ULMOD: {
			parse_single_stmt(parser, node, I_ULMOD);
		} break;
		case T_ULPRINT: {
			parse_single_stmt(parser, node, I_ULPRINT);
		} break;

		case T_IPUSH: {
			parse_push(parser, node, I_IPUSH);
		} break;
		case T_IADD: {
			parse_single_stmt(parser, node, I_IADD);
		} break;
		case T_ISUB: {
			parse_single_stmt(parser, node, I_ISUB);
		} break;
		case T_IMULT: {
			parse_single_stmt(parser, node, I_IMULT);
		} break;
		case T_IDIV: {
			parse_single_stmt(parser, node, I_IDIV);
		} break;
		case T_IMOD: {
			parse_single_stmt(parser, node, I_IMOD);
		} break;
		case T_IPRINT: {
			parse_single_stmt(parser, node, I_IPRINT);
		} break;

		case T_FPUSH: {
			parse_push(parser, node, I_FPUSH);
		} break;
		case T_FADD: {
			parse_single_stmt(parser, node, I_FADD);
		} break;
		case T_FSUB: {
			parse_single_stmt(parser, node, I_FSUB);
		} break;
		case T_FMULT: {
			parse_single_stmt(parser, node, I_FMULT);
		} break;
		case T_FDIV: {
			parse_single_stmt(parser, node, I_FDIV);
		} break;
		case T_FPRINT: {
			parse_single_stmt(parser, node, I_FPRINT);
		} break;

		case T_CPUSH: {
			parse_push(parser, node, I_CPUSH);
		} break;
		case T_CADD: {
			parse_single_stmt(parser, node, I_CADD);
		} break;
		case T_CSUB: {
			parse_single_stmt(parser, node, I_CSUB);
		} break;
		case T_CMULT: {
			parse_single_stmt(parser, node, I_CMULT);
		} break;
		case T_CDIV: {
			parse_single_stmt(parser, node, I_CDIV);
		} break;
		case T_CMOD: {
			parse_single_stmt(parser, node, I_CMOD);
		} break;
		case T_CPRINT: {
			parse_single_stmt(parser, node, I_CPRINT);
		} break;
		case T_CIPRINT: {
			parse_single_stmt(parser, node, I_CIPRINT);
		} break;

		case T_PPUSH: {
			parse_push(parser, node, I_PPUSH);
		} break;
		case T_PLOAD: {
			parse_idx(parser, node, I_PLOAD);
		} break;
		case T_PDEREF8: {
			parse_single_stmt(parser, node, I_PDEREF8);
		} break;
		case T_PDEREF32: {
			parse_single_stmt(parser, node, I_PDEREF32);
		} break;
		case T_PDEREF64: {
			parse_single_stmt(parser, node, I_PDEREF64);
		} break;
		case T_PDEREF: {
			parse_idx(parser, node, I_PDEREF);
		} break;
		case T_PSET8: {
			parse_single_stmt(parser, node, I_PSET8);
		} break;
		case T_PSET32: {
			parse_single_stmt(parser, node, I_PSET32);
		} break;
		case T_PSET64: {
			parse_single_stmt(parser, node, I_PSET64);
		} break;
		case T_PSET: {
			parse_idx(parser, node, I_PSET);
		} break;

		case T_POP8: {
			parse_single_stmt(parser, node, I_POP8);
		} break;
		case T_POP32: {
			parse_single_stmt(parser, node, I_POP32);
		} break;
		case T_POP64: {
			parse_single_stmt(parser, node, I_POP64);
		} break;

		case T_JUMP: {
			parse_jump(parser, node);
		} break;
		case T_JUMPCMP: {
			parse_jumpcmp(parser, node, I_JUMPCMP);
		} break;
		case T_JUMPPROC: {
			parse_jumpproc(parser, node);
		} break;

		case T_ICLT: {
			parse_single_stmt(parser, node, I_ICLT);
		} break;
		case T_ICLE: {
			parse_single_stmt(parser, node, I_ICLE);
		} break;
		case T_ICEQ: {
			parse_single_stmt(parser, node, I_ICEQ);
		} break;
		case T_ICGT: {
			parse_single_stmt(parser, node, I_ICGT);
		} break;
		case T_ICGE: {
			parse_single_stmt(parser, node, I_ICGE);
		} break;

		case T_ULCLT: {
			parse_single_stmt(parser, node, I_ULCLT);
		} break;
		case T_ULCLE: {
			parse_single_stmt(parser, node, I_ULCLE);
		} break;
		case T_ULCEQ: {
			parse_single_stmt(parser, node, I_ULCEQ);
		} break;
		case T_ULCGT: {
			parse_single_stmt(parser, node, I_ULCGT);
		} break;
		case T_ULCGE: {
			parse_single_stmt(parser, node, I_ULCGE);
		} break;

		case T_FCLT: {
			parse_single_stmt(parser, node, I_FCLT);
		} break;
		case T_FCLE: {
			parse_single_stmt(parser, node, I_FCLE);
		} break;
		case T_FCEQ: {
			parse_single_stmt(parser, node, I_FCEQ);
		} break;
		case T_FCGT: {
			parse_single_stmt(parser, node, I_FCGT);
		} break;
		case T_FCGE: {
			parse_single_stmt(parser, node, I_FCGE);
		} break;

		case T_CCLT: {
			parse_single_stmt(parser, node, I_CCLT);
		} break;
		case T_CCLE: {
			parse_single_stmt(parser, node, I_CCLE);
		} break;
		case T_CCEQ: {
			parse_single_stmt(parser, node, I_CCEQ);
		} break;
		case T_CCGT: {
			parse_single_stmt(parser, node, I_CCGT);
		} break;
		case T_CCGE: {
			parse_single_stmt(parser, node, I_CCGE);
		} break;

		case T_LABEL: {
			parse_label(parser, &token, node);
		} break;

		case T_DUPE8: {
			parse_single_stmt(parser, node, I_DUPE8);
		} break;
		case T_DUPE32: {
			parse_single_stmt(parser, node, I_DUPE32);
		} break;
		case T_DUPE64: {
			parse_single_stmt(parser, node, I_DUPE64);
		} break;

		case T_SWAP8: {
			parse_single_stmt(parser, node, I_SWAP8);
		} break;
		case T_SWAP32: {
			parse_single_stmt(parser, node, I_SWAP32);
		} break;
		case T_SWAP64: {
			parse_single_stmt(parser, node, I_SWAP64);
		} break;

		case T_COPY8: {
			parse_idx(parser, node, I_COPY8);
		} break;
		case T_COPY32: {
			parse_idx(parser, node, I_COPY32);
		} break;
		case T_COPY64: {
			parse_idx(parser, node, I_COPY64);
		} break;

		case T_STORE8: {
			parse_idx(parser, node, I_STORE8);
		} break;
		case T_STORE32: {
			parse_idx(parser, node, I_STORE32);
		} break;
		case T_STORE64: {
			parse_idx(parser, node, I_STORE64);
		} break;

		case T_LOAD8: {
			parse_idx(parser, node, I_LOAD8);
		} break;
		case T_LOAD32: {
			parse_idx(parser, node, I_LOAD32);
		} break;
		case T_LOAD64: {
			parse_idx(parser, node, I_LOAD64);
		} break;

		case T_RET8: {
			parse_single_stmt(parser, node, I_RET8);
		} break;
		case T_RET32: {
			parse_single_stmt(parser, node, I_RET32);
		} break;
		case T_RET64: {
			parse_single_stmt(parser, node, I_RET64);
		} break;

		case T_RET: {
			parse_idx(parser, node, I_RET);
		} break;

		case T_EOF: {
			node->kind = N_EOF;
		} break;
		default: {
			node->kind = N_ILLEGAL;
			char msg[256] = {0};
			char name[256] = {0};
			token_name(&token, name);
			sprintf(msg, "Unexpected start of statement:%s", name);
			parser_err(parser, msg);
			goto error;
		}
		}
	} break;
	}

error:

#if DEBUG
	if (parser->state == PARSE_DATA) {
		assert(node->kind != N_INSTRUCTION);
	} else if (parser->state == PARSE_TEXT) {
		assert(node->kind != N_DECLARATION);
	} else {
		panic("parser in illegal state:%d\n", parser->state);
	}
#endif

	return node;
}
