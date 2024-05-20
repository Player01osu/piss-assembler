#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

#ifdef DEBUG_TRACE_GNU
#define span_join(a, b) ({printf("%s:%d:span_join(a, b)\n", __FILE__, __LINE__); span_join(a, b);})
#endif

void parser_init(Parser *parser, Arena *arena, FILE *file, size_t len)
{
	Lexer lexer = {0};
	lexer_init(&lexer, arena, file, len);
	parser->span = (Span) { 0, 0, 0, 0 };
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

#define parser_err(parser, msg)                                       \
	do {                                                          \
		size_t len = strlen(msg) + 1;                         \
		char *_s = (char *) arena_xalloc(parser->arena, len); \
		memcpy(_s, msg, len);                                 \
		parser->error = _s;                                   \
	} while (0)

#define single_stmt_expect(parser, instruction_kind)                 \
	do {                                                         \
		Token next = parser_bump(parser);                    \
		if (next.kind != T_EOL && next.kind != T_EOF) {      \
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
		parser_err(parser, "Expected Literal or Ident");
		return -1;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
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
		parser_err(parser, "Expected index");
		return -1;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
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
		parser_err(parser, "Expected label");
		return -1;
	}

	next = parser_bump(parser);
	node->span = span_join(node->span, next.span);
	if (next.kind == T_UINUMLIT) {
		node->data.instruction.data.proc.argc = next.data.ui;
	} else {
		parser_err(parser, "Expected number of bytes");
		return -1;
	}

	next = parser_bump(parser);
	if (!is_end_of_statement(next.kind)) {
		parser_err(parser, "Expected newline");
		return -1;
	}
	return 0;
}

int parse_label(Parser *parser, const Token *token, Node *node)
{
	Token next = parser_bump(parser);
	node->span = span_join(node->span, token->span);
	if (!is_end_of_statement(next.kind)) {
		char __s[256];
		sprintf(__s, "%s:Expected end of statement\n", __func__);
		parser_err(parser, __s);
		return -1;
	}
	node->kind = N_LABEL;
	node->data.s = token->data.s;
	return 0;
}

int parse_data(Parser *parser, const Token *token, Node *node)
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
		case T_DD:
			node->data.declaration.kind = D_DD;
			ty_size = 8;
			break;
		case T_DB:
			node->data.declaration.kind = D_DB;
			ty_size = 1;
			break;
		case T_DW:
			node->data.declaration.kind = D_DW;
			ty_size = 4;
			break;
		default:
			panic("Unreachable\n");
		}

		next = parser_bump(parser);
		if (next.kind == T_OPEN_BRACKET) {
			next = parser_bump(parser);
			if (next.kind != T_UINUMLIT) {
				parser_err(parser, "expected an unsigned integer");
				return -1;
			}

			node->data.declaration.bytes = calloc(next.data.ui, ty_size);
			node->data.declaration.len = next.data.ui * ty_size;
			node->span = span_join(node->span, next.span);
			node->data.declaration.span = node->span;

			next = parser_bump(parser);
			if (next.kind != T_CLOSE_BRACKET) {
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
			char __s[256];
			sprintf(__s, "%s:Expected end of statement\n", __func__);
			parser_err(parser, __s);
			return -1;
		}
	} else {
		char __s[256];
		sprintf(__s, "%s:Expected data kind\n", __func__);
		parser_err(parser, __s);
		return -1;
	}

	return 0;
}

Node *parser_next(Parser *parser)
{
	Token token;
	Node *node = arena_xalloc(parser->arena, sizeof(*node));
	int errcode = 0;

tailcall:
	token = parser_bump(parser);
	node->span = token.span;

	switch (token.kind) {
	case T_SECTION_TEXT:
		parser->state = PARSE_TEXT;
		goto tailcall;
	case T_SECTION_DATA:
		parser->state = PARSE_DATA;
		goto tailcall;
	case T_EOL:
		goto tailcall;
	default: ;
	}

	switch (parser->state) {
	case PARSE_DATA:
		switch (token.kind) {
		case T_IDENT:
			errcode = parse_data(parser, &token, node);
			break;
		default: {
			char msg[256] = {0};
			char name[256] = {0};
			token_name(&token, name);
			sprintf(msg, "Unexpected start of statement:%s", name);
			parser_err(parser, msg);
			errcode = -1;
		}
		}
		break;
	case PARSE_TEXT:
		switch (token.kind) {
		case T_ULPUSH:
			errcode = parse_push(parser, node, I_ULPUSH);
			break;
		case T_ULADD:
			errcode = parse_single_stmt(parser, node, I_ULADD);
			break;
		case T_ULSUB:
			errcode = parse_single_stmt(parser, node, I_ULSUB);
			break;
		case T_ULMULT:
			errcode = parse_single_stmt(parser, node, I_ULMULT);
			break;
		case T_ULDIV:
			errcode = parse_single_stmt(parser, node, I_ULDIV);
			break;
		case T_ULMOD:
			errcode = parse_single_stmt(parser, node, I_ULMOD);
			break;
		case T_ULPRINT:
			errcode = parse_single_stmt(parser, node, I_ULPRINT);
			break;

		case T_IPUSH:
			errcode = parse_push(parser, node, I_IPUSH);
			break;
		case T_IADD:
			errcode = parse_single_stmt(parser, node, I_IADD);
			break;
		case T_ISUB:
			errcode = parse_single_stmt(parser, node, I_ISUB);
			break;
		case T_IMULT:
			errcode = parse_single_stmt(parser, node, I_IMULT);
			break;
		case T_IDIV:
			errcode = parse_single_stmt(parser, node, I_IDIV);
			break;
		case T_IMOD:
			errcode = parse_single_stmt(parser, node, I_IMOD);
			break;
		case T_IPRINT:
			errcode = parse_single_stmt(parser, node, I_IPRINT);
			break;

		case T_FPUSH:
			errcode = parse_push(parser, node, I_FPUSH);
			break;
		case T_FADD:
			errcode = parse_single_stmt(parser, node, I_FADD);
			break;
		case T_FSUB:
			errcode = parse_single_stmt(parser, node, I_FSUB);
			break;
		case T_FMULT:
			errcode = parse_single_stmt(parser, node, I_FMULT);
			break;
		case T_FDIV:
			errcode = parse_single_stmt(parser, node, I_FDIV);
			break;
		case T_FPRINT:
			errcode = parse_single_stmt(parser, node, I_FPRINT);
			break;

		case T_CPUSH:
			errcode = parse_push(parser, node, I_CPUSH);
			break;
		case T_CADD:
			errcode = parse_single_stmt(parser, node, I_CADD);
			break;
		case T_CSUB:
			errcode = parse_single_stmt(parser, node, I_CSUB);
			break;
		case T_CMULT:
			errcode = parse_single_stmt(parser, node, I_CMULT);
			break;
		case T_CDIV:
			errcode = parse_single_stmt(parser, node, I_CDIV);
			break;
		case T_CMOD:
			errcode = parse_single_stmt(parser, node, I_CMOD);
			break;
		case T_CPRINT:
			errcode = parse_single_stmt(parser, node, I_CPRINT);
			break;
		case T_CIPRINT:
			errcode = parse_single_stmt(parser, node, I_CIPRINT);
			break;

		case T_PPUSH:
			errcode = parse_push(parser, node, I_PPUSH);
			break;
		case T_PLOAD:
			errcode = parse_idx(parser, node, I_PLOAD);
			break;
		case T_PDEREF8:
			errcode = parse_single_stmt(parser, node, I_PDEREF8);
			break;
		case T_PDEREF32:
			errcode = parse_single_stmt(parser, node, I_PDEREF32);
			break;
		case T_PDEREF64:
			errcode = parse_single_stmt(parser, node, I_PDEREF64);
			break;
		case T_PDEREF:
			errcode = parse_idx(parser, node, I_PDEREF);
			break;
		case T_PSET8:
			errcode = parse_single_stmt(parser, node, I_PSET8);
			break;
		case T_PSET32:
			errcode = parse_single_stmt(parser, node, I_PSET32);
			break;
		case T_PSET64:
			errcode = parse_single_stmt(parser, node, I_PSET64);
			break;
		case T_PSET:
			errcode = parse_idx(parser, node, I_PSET);
			break;

		case T_POP8:
			errcode = parse_single_stmt(parser, node, I_POP8);
			break;
		case T_POP32:
			errcode = parse_single_stmt(parser, node, I_POP32);
			break;
		case T_POP64:
			errcode = parse_single_stmt(parser, node, I_POP64);
			break;

		case T_JUMP:
			errcode = parse_jump(parser, node);
			break;
		case T_JUMPCMP:
			errcode = parse_jumpcmp(parser, node, I_JUMPCMP);
			break;
		case T_JUMPPROC:
			errcode = parse_jumpproc(parser, node);
			break;

		case T_ICLT:
			errcode = parse_single_stmt(parser, node, I_ICLT);
			break;
		case T_ICLE:
			errcode = parse_single_stmt(parser, node, I_ICLE);
			break;
		case T_ICEQ:
			errcode = parse_single_stmt(parser, node, I_ICEQ);
			break;
		case T_ICGT:
			errcode = parse_single_stmt(parser, node, I_ICGT);
			break;
		case T_ICGE:
			errcode = parse_single_stmt(parser, node, I_ICGE);
			break;

		case T_ULCLT:
			errcode = parse_single_stmt(parser, node, I_ULCLT);
			break;
		case T_ULCLE:
			errcode = parse_single_stmt(parser, node, I_ULCLE);
			break;
		case T_ULCEQ:
			errcode = parse_single_stmt(parser, node, I_ULCEQ);
			break;
		case T_ULCGT:
			errcode = parse_single_stmt(parser, node, I_ULCGT);
			break;
		case T_ULCGE:
			errcode = parse_single_stmt(parser, node, I_ULCGE);
			break;

		case T_FCLT:
			errcode = parse_single_stmt(parser, node, I_FCLT);
			break;
		case T_FCLE:
			errcode = parse_single_stmt(parser, node, I_FCLE);
			break;
		case T_FCEQ:
			errcode = parse_single_stmt(parser, node, I_FCEQ);
			break;
		case T_FCGT:
			errcode = parse_single_stmt(parser, node, I_FCGT);
			break;
		case T_FCGE:
			errcode = parse_single_stmt(parser, node, I_FCGE);
			break;

		case T_CCLT:
			errcode = parse_single_stmt(parser, node, I_CCLT);
			break;
		case T_CCLE:
			errcode = parse_single_stmt(parser, node, I_CCLE);
			break;
		case T_CCEQ:
			errcode = parse_single_stmt(parser, node, I_CCEQ);
			break;
		case T_CCGT:
			errcode = parse_single_stmt(parser, node, I_CCGT);
			break;
		case T_CCGE:
			errcode = parse_single_stmt(parser, node, I_CCGE);
			break;

		case T_LABEL:
			errcode = parse_label(parser, &token, node);
			break;

		case T_DUPE8:
			errcode = parse_single_stmt(parser, node, I_DUPE8);
			break;
		case T_DUPE32:
			errcode = parse_single_stmt(parser, node, I_DUPE32);
			break;
		case T_DUPE64:
			errcode = parse_single_stmt(parser, node, I_DUPE64);
			break;

		case T_SWAP8:
			errcode = parse_single_stmt(parser, node, I_SWAP8);
			break;
		case T_SWAP32:
			errcode = parse_single_stmt(parser, node, I_SWAP32);
			break;
		case T_SWAP64:
			errcode = parse_single_stmt(parser, node, I_SWAP64);
			break;

		case T_COPY8:
			errcode = parse_idx(parser, node, I_COPY8);
			break;
		case T_COPY32:
			errcode = parse_idx(parser, node, I_COPY32);
			break;
		case T_COPY64:
			errcode = parse_idx(parser, node, I_COPY64);
			break;

		case T_STORE8:
			errcode = parse_idx(parser, node, I_STORE8);
			break;
		case T_STORE32:
			errcode = parse_idx(parser, node, I_STORE32);
			break;
		case T_STORE64:
			errcode = parse_idx(parser, node, I_STORE64);
			break;

		case T_LOAD8:
			errcode = parse_idx(parser, node, I_LOAD8);
			break;
		case T_LOAD32:
			errcode = parse_idx(parser, node, I_LOAD32);
			break;
		case T_LOAD64:
			errcode = parse_idx(parser, node, I_LOAD64);
			break;

		case T_RET8:
			errcode = parse_single_stmt(parser, node, I_RET8);
			break;
		case T_RET32:
			errcode = parse_single_stmt(parser, node, I_RET32);
			break;
		case T_RET64:
			errcode = parse_single_stmt(parser, node, I_RET64);
			break;

		case T_RET:
			errcode = parse_idx(parser, node, I_RET);
			break;

		case T_EOF:
			node->kind = N_EOF;
			break;
		default: {
			char msg[256] = {0};
			char name[256] = {0};
			token_name(&token, name);
			sprintf(msg, "Unexpected start of statement:%s", name);
			parser_err(parser, msg);
			errcode = -1;
		}
		}
		break;
	}

	if (errcode < 0) {
		parser->span = node->span;
		return NULL;
	}

#ifdef DEBUG
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
