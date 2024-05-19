#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ARENA_IMPLEMENTATION
#define ARENA_DEFAULT_ALIGNMENT sizeof(size_t)
#include "arena.h"
#undef ARENA_IMPLEMENTATION

#include "lexer.h"
#include "parser.h"
#include "ass.h"

const char *HELP = "Usage: ass [options] file ...\n";

typedef struct FramePointer {
	byte *ptr;
	byte *return_stack_ptr;
	byte *start;
	byte locals[LOCAL_SIZE];
	struct FramePointer *prev;
} FramePointer;

typedef struct Ctx {
	byte stack[STACK_SIZE];
	byte return_stack[STACK_SIZE];
	FramePointer *frame_ptr;
	size_t pc;

	DeclarationMap declaration_map;
	LabelMap label_map;

	Instruction **instructions;
	size_t instruction_cap;
	size_t instruction_len;
} Ctx;

void *_arena_xalloc(char *filename, int row, Arena *arena, size_t size)
{
	if (arena->index + size > arena->size) {
		size_t old_size = arena->size;
		size_t new_size = arena->size * 2;
#ifdef DEBUG_TRACE
		fprintf(stderr, "%s:%d:DEBUG:attempting to growing arena... %lu => %lu\n", filename, row, old_size, new_size);
#endif
		if (!arena_expand(arena, new_size)) {
			fprintf(stderr, "%s:%d:ERROR:failed to expand arena... %lu => %lu\n", filename, row, old_size, new_size);
			exit(1);
		}
	}

	void *block = arena_alloc(arena, size);
	assert(block);
	return block;
}

void *_xmalloc(char *filename, int row, size_t size)
{
	void *block = malloc(size);
	if (!block) {
		fprintf(stderr, "%s:%d:ERROR:failed to allocate block... %lu\n", filename, row, size);
		exit(1);
	}
	return block;
}

void *_xrealloc(char *filename, int row, void *ptr, size_t size)
{
	void *block = realloc(ptr, size);
	if (!block) {
		fprintf(stderr, "%s:%d:ERROR:failed to allocate block... %lu\n", filename, row, size);
		exit(1);
	}
	return block;
}

void print_help(void)
{
	fputs(HELP, stdout);
}

bool insert_label(LabelMap *label_map, const char *name, size_t location)
{
	if (label_map->len >= label_map->cap) {
		label_map->cap *= 2;
		label_map->labels =
			xrealloc(label_map->labels, sizeof(Label) * label_map->cap);
	}
	label_map->labels[label_map->len++] = (Label){
		.name = name,
		.location = location,
	};
	return true;
}

bool insert_declaration(DeclarationMap *declaration_map, Declaration declaration)
{
	if (declaration_map->len >= declaration_map->cap) {
		declaration_map->cap *= 2;
		declaration_map->declarations =
			xrealloc(declaration_map->declarations, sizeof(*declaration_map->declarations) * declaration_map->cap);
	}
	declaration_map->declarations[declaration_map->len++] = declaration;
	return true;
}

FramePointer *context_push_frame(Ctx *context)
{
	FramePointer *p = xmalloc(sizeof(FramePointer));
	if (context->frame_ptr) {
		p->prev = context->frame_ptr;
		p->start = context->frame_ptr->ptr;
		p->ptr = context->frame_ptr->ptr;
	}
	context->frame_ptr = p;
	return context->frame_ptr;
}

FramePointer *context_pop_frame(Ctx *context)
{
	if (!context->frame_ptr->prev) return NULL;
	FramePointer *p = context->frame_ptr;
	context->frame_ptr = context->frame_ptr->prev;
	free(p);
	return context->frame_ptr;
}

Instruction *process_node(Ctx *context, Node *node)
{
	switch (node->kind) {
	case N_INSTRUCTION:
		return &node->data.instruction;
	case N_LABEL:
		if (!insert_label(&context->label_map, node->data.s, context->pc)) {
			panic("Failed to create label");
		}
		return NULL;
	case N_DECLARATION:
		if (!insert_declaration(&context->declaration_map, node->data.declaration)) {
			panic("Failed to create declaration");
		}
		return NULL;
	default:
		panic("Unimplemented");
	}
}

bool empty_stack(Ctx *context)
{
	return context->frame_ptr->ptr <= context->frame_ptr->start;
}

void dump_stack(Ctx *context)
{
	for (size_t i = 0; i < STACK_SIZE; ++i) {
		fprintf(stderr, "%d,", context->stack[i]);
	}
}

void push_stack(Ctx *context, void *data, size_t size)
{
	if (context->frame_ptr->ptr - context->stack + size >= STACK_SIZE) {
		dump_stack(context);
		panic("Stack overflow! Dumping stack\n");
	}
	byte *ptr = context->frame_ptr->ptr;
	memcpy(ptr, data, size);
	context->frame_ptr->ptr += size;
}

void *pop_stack(Ctx *context, size_t n)
{
	context->frame_ptr->ptr -= n;
	byte *top = context->frame_ptr->ptr;
	return top;
}

#define TYOP_INST(ty, prefix, printf_str) \
	case I_##prefix##PUSH: {                              \
		uint64_t item = instruction->data.lit.data.i; \
		push_stack(context, &item, sizeof(ty));       \
		break;                                        \
	}                                                     \
	case I_##prefix##ADD: {                               \
		STACK_CHECK;                                  \
		ty *b = pop_stack(context, sizeof(*b));       \
		STACK_CHECK;                                  \
		ty *a = pop_stack(context, sizeof(*a));       \
                                                              \
		ty item = *a + *b;                            \
		push_stack(context, &item, sizeof(item));     \
		break;                                        \
	}                                                     \
	case I_##prefix##SUB: {                               \
		STACK_CHECK;                                  \
		ty *b = pop_stack(context, sizeof(*b));       \
		STACK_CHECK;                                  \
		ty *a = pop_stack(context, sizeof(*a));       \
                                                              \
		ty item = *a - *b;                            \
		push_stack(context, &item, sizeof(item));     \
		break;                                        \
	}                                                     \
	case I_##prefix##MULT: {                              \
		STACK_CHECK;                                  \
		ty *b = pop_stack(context, sizeof(*b));       \
		STACK_CHECK;                                  \
		ty *a = pop_stack(context, sizeof(*a));       \
                                                              \
		ty item = *a * *b;                            \
		push_stack(context, &item, sizeof(item));     \
		break;                                        \
	}                                                     \
	case I_##prefix##DIV: {                               \
		STACK_CHECK;                                  \
		ty *b = pop_stack(context, sizeof(*b));       \
		STACK_CHECK;                                  \
		ty *a = pop_stack(context, sizeof(*a));       \
                                                              \
		ty item = *a / *b;                            \
		push_stack(context, &item, sizeof(item));     \
		break;                                        \
	}                                                     \
	case I_##prefix##PRINT: {                             \
		STACK_CHECK;                                  \
		byte *stack_ptr = context->frame_ptr->ptr;    \
		ty *a = (ty *)(&stack_ptr[-sizeof(*a)]);      \
		printf(printf_str, *a);                       \
		break;                                        \
	}                                                     \
	case I_##prefix##CEQ: {                               \
		STACK_CHECK;                                  \
		byte *stack_ptr = context->frame_ptr->ptr;    \
		ty *b = (ty *)(&stack_ptr[-(sizeof(*b) * 1)]);\
		ty *a = (ty *)(&stack_ptr[-(sizeof(*a) * 2)]);\
		bool item = *a == *b;                         \
		push_stack(context, &item, 1);                \
		break;                                        \
	}                                                     \
	case I_##prefix##CLT: {                               \
		STACK_CHECK;                                  \
		byte *stack_ptr = context->frame_ptr->ptr;    \
		ty *b = (ty *)&stack_ptr[-(sizeof(*b) * 1)];  \
		ty *a = (ty *)&stack_ptr[-(sizeof(*a) * 2)];  \
		bool item = *a < *b;                          \
		push_stack(context, &item, 1);                \
		break;                                        \
	}                                                     \
	case I_##prefix##CLE: {                               \
		STACK_CHECK;                                  \
		byte *stack_ptr = context->frame_ptr->ptr;    \
		ty *b = (ty *)&stack_ptr[-(sizeof(*b) * 1)];  \
		ty *a = (ty *)&stack_ptr[-(sizeof(*a) * 2)];  \
		bool item = *a <= *b;                         \
		push_stack(context, &item, 1);                \
		break;                                        \
	}                                                     \
	case I_##prefix##CGT: {                               \
		STACK_CHECK;                                  \
		byte *stack_ptr = context->frame_ptr->ptr;    \
		ty *b = (ty *)&stack_ptr[-(sizeof(*b) * 1)];  \
		ty *a = (ty *)&stack_ptr[-(sizeof(*a) * 2)];  \
		bool item = *a > *b;                          \
		push_stack(context, &item, 1);                \
		break;                                        \
	}                                                     \
	case I_##prefix##CGE: {                               \
		STACK_CHECK;                                  \
		byte *stack_ptr = context->frame_ptr->ptr;    \
		ty *b = (ty *)&stack_ptr[-(sizeof(*b) * 1)];  \
		ty *a = (ty *)&stack_ptr[-(sizeof(*a) * 2)];  \
		bool item = *a >= *b;                         \
		push_stack(context, &item, 1);                \
		break;                                        \
	}

/* Integer types also have `mod` operation */
#define ITYOP_INST(ty, prefix, printf_str)                \
	TYOP_INST(ty, prefix, printf_str)                 \
	case I_##prefix##MOD: {                           \
		STACK_CHECK;                              \
		ty *b = pop_stack(context, sizeof(*b));   \
		STACK_CHECK;                              \
		ty *a = pop_stack(context, sizeof(*a));   \
                                                          \
		ty item = *a % *b;                        \
		push_stack(context, &item, sizeof(item)); \
		break;                                    \
	}                                                 \

void exec_instruction(Ctx *context, Instruction *instruction)
{
#define STACK_CHECK do { if (empty_stack(context)) goto empty_stack; } while(0)

	switch (instruction->kind) {
	case I_PPUSH: {
		void *item = instruction->data.ptr;
		push_stack(context, &item, sizeof(item));
		break;
	}
	case I_PLOAD: {
		void *data_ptr = instruction->data.ptr;
		push_stack(context, &data_ptr, sizeof(data_ptr));

		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		push_stack(context, &slot, sizeof(slot));
		break;
	}
	case I_PDEREF8: {
		STACK_CHECK;
		char **item = pop_stack(context, sizeof(*item));
		push_stack(context, *item, sizeof(**item));
		break;
	}
	case I_PDEREF32: {
		STACK_CHECK;
		int **item = pop_stack(context, sizeof(*item));
		push_stack(context, *item, sizeof(**item));
		break;
	}
	case I_PDEREF64: {
		STACK_CHECK;
		long **item = pop_stack(context, sizeof(*item));
		push_stack(context, *item, sizeof(**item));
		break;
	}
	case I_PSET8: {
		STACK_CHECK;
		char **a = pop_stack(context, sizeof(*a));
		STACK_CHECK;
		char *b = pop_stack(context, sizeof(*b));
		**a = *b;
		break;
	}
	case I_PSET32: {
		STACK_CHECK;
		int **a = pop_stack(context, sizeof(*a));
		STACK_CHECK;
		int *b = pop_stack(context, sizeof(*b));
		**a = *b;
		break;
	}
	case I_PSET64: {
		STACK_CHECK;
		long **a = pop_stack(context, sizeof(*a));
		STACK_CHECK;
		long *b = pop_stack(context, sizeof(*b));
		**a = *b;
		break;
	}
	ITYOP_INST(unsigned long, UL, "%lu")
	ITYOP_INST(int, I, "%d")
	ITYOP_INST(char, C, "%c")
	TYOP_INST(float, F, "%f")
#undef TYOP_INST
#undef ITYOP_INST

	case I_CIPRINT: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		char *a = (char *)(&stack_ptr[-sizeof(*a)]);
		printf("%d", *a);
		break;
	}


	case I_POP8: {
		STACK_CHECK;
		pop_stack(context, 1);
		break;
	}
	case I_POP32: {
		STACK_CHECK;
		pop_stack(context, 4);
		break;
	}
	case I_POP64: {
		STACK_CHECK;
		pop_stack(context, 8);
		break;
	}
	case I_SWAP8: {
		STACK_CHECK;
		byte *a = pop_stack(context, 1);
		byte *b = pop_stack(context, 1);
		byte tmp[1] = {0};
		memcpy(tmp, b, 1);
		push_stack(context, a, 1);
		push_stack(context, tmp, 1);
		break;
	}
	case I_SWAP32: {
		STACK_CHECK;
		byte *a = pop_stack(context, 4);
		byte *b = pop_stack(context, 4);
		byte tmp[4] = {0};
		memcpy(tmp, b, 4);
		push_stack(context, a, 4);
		push_stack(context, tmp, 4);
		break;
	}
	case I_SWAP64: {
		STACK_CHECK;
		byte *a = pop_stack(context, 8);
		byte *b = pop_stack(context, 8);
		byte tmp[8] = {0};
		memcpy(tmp, b, 8);
		push_stack(context, a, 8);
		push_stack(context, tmp, 8);
		break;
	}
	case I_DUPE8: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		byte *a = &stack_ptr[-1];
		push_stack(context, a, 1);
		break;
	}
	case I_DUPE32: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		byte *a = &stack_ptr[-4];
		push_stack(context, a, 4);
		break;
	}
	case I_DUPE64: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		byte *a = &stack_ptr[-8];
		push_stack(context, a, 8);
		break;
	}
	case I_COPY8: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		byte *a = &stack_ptr[-1];
		size_t n = instruction->data.n;
		for (size_t i = 0; i < n; ++i) {
			push_stack(context, a, 1);
		}
		break;
	}
	case I_COPY32: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		byte *a = &stack_ptr[-4];
		size_t n = instruction->data.n;
		for (size_t i = 0; i < n; ++i) {
			push_stack(context, a, 4);
		}
		break;
	}
	case I_COPY64: {
		STACK_CHECK;
		byte *stack_ptr = context->frame_ptr->ptr;
		byte *a = &stack_ptr[-8];
		size_t n = instruction->data.n;
		for (size_t i = 0; i < n; ++i) {
			push_stack(context, a, 8);
		}
		break;
	}
	case I_STORE8: {
		STACK_CHECK;
		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		byte *a = pop_stack(context, 1);
		memcpy(slot, a, 1);
		break;
	}
	case I_STORE32: {
		STACK_CHECK;
		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		byte *a = pop_stack(context, 4);
		memcpy(slot, a, 4);
		break;
	}
	case I_STORE64: {
		STACK_CHECK;
		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		byte *a = pop_stack(context, 8);
		memcpy(slot, a, 8);
		break;
	}
	case I_LOAD8: {
		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		push_stack(context, slot, 1);
		break;
	}
	case I_LOAD32: {
		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		push_stack(context, slot, 4);
		break;
	}
	case I_LOAD64: {
		size_t n = instruction->data.n;
		byte *locals = context->frame_ptr->locals;
		byte *slot = &locals[n];
		push_stack(context, slot, 8);
		break;
	}
	case I_RET8: {
		FramePointer *stack_ptr = context->frame_ptr;
		FramePointer *stack_ptr_prev = stack_ptr->prev;
		size_t return_addr = *(size_t *)(&stack_ptr->return_stack_ptr[-sizeof(size_t)]);
		byte *a = pop_stack(context, 1);
		byte tmp[1] = {0};
		memcpy(tmp, a, 1);
		context->frame_ptr = stack_ptr_prev;
		context->pc = return_addr;
		free(stack_ptr);
		push_stack(context, tmp, 1);
		break;
	}
	case I_RET32: {
		FramePointer *stack_ptr = context->frame_ptr;
		FramePointer *stack_ptr_prev = stack_ptr->prev;
		size_t return_addr = *(size_t *)(&stack_ptr->return_stack_ptr[-sizeof(size_t)]);
		byte *a = pop_stack(context, 4);
		byte tmp[4] = {0};
		memcpy(tmp, a, 4);
		context->frame_ptr = stack_ptr_prev;
		context->pc = return_addr;
		free(stack_ptr);
		push_stack(context, tmp, 4);
		break;
	}
	case I_RET64: {
		FramePointer *stack_ptr = context->frame_ptr;
		FramePointer *stack_ptr_prev = stack_ptr->prev;
		size_t return_addr = *(size_t *)(&stack_ptr->return_stack_ptr[-sizeof(size_t)]);
		byte *a = pop_stack(context, 8);
		byte tmp[8] = {0};
		memcpy(tmp, a, 8);
		context->frame_ptr = stack_ptr_prev;
		context->pc = return_addr;
		free(stack_ptr);
		push_stack(context, tmp, 8);
		break;
	}
	case I_RET: {
		FramePointer *stack_ptr = context->frame_ptr;
		FramePointer *stack_ptr_prev = stack_ptr->prev;
		size_t n = instruction->data.n;
		size_t return_addr = *(size_t *)(&stack_ptr->return_stack_ptr[-sizeof(size_t)]);
		byte *a = pop_stack(context, n);
		byte tmp[n];
		memcpy(tmp, a, n);
		context->frame_ptr = stack_ptr_prev;
		context->pc = return_addr;
		free(stack_ptr);
		push_stack(context, tmp, n);
		break;
	}
	case I_JUMPPROC: {
		size_t argc = instruction->data.proc.argc;
		FramePointer *stack_ptr = context->frame_ptr;
		FramePointer *stack_ptr_new = xmalloc(sizeof(FramePointer));
		// Move previous stack frame
		stack_ptr->ptr -= argc;
		stack_ptr_new->ptr = stack_ptr->ptr;
		stack_ptr_new->start = stack_ptr->start;
		stack_ptr_new->return_stack_ptr = stack_ptr->return_stack_ptr;
		stack_ptr_new->prev = stack_ptr;
		context->frame_ptr = stack_ptr_new;
		// Push pc onto return stack
		*(size_t *)stack_ptr_new->return_stack_ptr = context->pc;
		stack_ptr_new->return_stack_ptr += sizeof(size_t);

		context->pc += instruction->data.proc.location.offset;
		// Initial locals with args
		memcpy(stack_ptr_new->locals, stack_ptr_new->ptr, argc);
		break;
	}
	case I_JUMP: {
		context->pc += instruction->data.offset;
		break;
	}
	case I_JUMPCMP: {
		STACK_CHECK;
		byte *ptr = &context->frame_ptr->ptr[-1];
		if (*ptr) {
			context->pc += instruction->data.offset;
			break;
		}
		break;
	}
	default:
		fprintf(stderr, "%s: {%d:UNIMPLMEMENTED:%d\n", __FILE__, __LINE__, instruction->kind);
	}

	return;
empty_stack:
	puts("Stack is empty\n");
	return;

#undef STACK_CHECK
}

void context_init(Ctx *context)
{
	context_push_frame(context);
	context->frame_ptr->ptr = context->stack;
	context->frame_ptr->start = context->stack;
	context->frame_ptr->return_stack_ptr = context->return_stack;
	context->frame_ptr->prev = NULL;

	context->instructions = xmalloc(sizeof(*context->instructions) * 16);
	context->instruction_cap = 16;
	context->instruction_len = 0;

	context->label_map = (LabelMap){
		.labels = xmalloc(sizeof(*context->label_map.labels) * 16),
		.cap = 16,
		.len = 0,
	};

	context->declaration_map = (DeclarationMap){
		.declarations = xmalloc(sizeof(*context->declaration_map.declarations) * 16),
		.cap = 16,
		.len = 0,
	};

	context->pc = 0;
}

void context_destroy(Ctx *context)
{
	while (context->frame_ptr) {
		FramePointer *p = context->frame_ptr;
		context->frame_ptr = context->frame_ptr->prev;
		free(p);
	}
	free(context->label_map.labels);
	free(context->declaration_map.declarations);
	free(context->instructions);
}

void context_push_instruction(Ctx *context, Instruction *instruction)
{
	if (context->instruction_len >= context->instruction_cap) {
		context->instruction_cap *= 2;
		context->instructions =
			xrealloc(context->instructions, sizeof(*context->instructions) * context->instruction_cap);
	}
	context->instructions[context->instruction_len++] = instruction;
}

// TODO: Use a map
bool label_location(Ctx *context, const char *label_name, ssize_t *location)
{
	LabelMap *label_map = &context->label_map;
	for (size_t i = 0; i < label_map->len; ++i) {
		Label *label = &label_map->labels[i];
		// TODO: Intern
		if (strcmp(label_name, label->name) == 0) {
			*location = label->location;
			return true;
		}
	}
	return false;
}

bool resolve_load(Ctx *context, const char *data_name, void **data_ptr)
{
	DeclarationMap *declaration_map = &context->declaration_map;
	for (size_t i = 0; i < declaration_map->len; ++i) {
		Declaration *declaration = &declaration_map->declarations[i];
		// TODO: Intern
		if (strcmp(data_name, declaration->ident) == 0) {
			*data_ptr = declaration->bytes;
			return true;
		}
	}
	return false;
}

void begin_execution(Ctx *context)
{
	while (context->pc < context->instruction_len) {
		exec_instruction(context, context->instructions[context->pc++]);
	}
}

int parse_src(Ctx *context, Arena *arena, const char *filename, FILE *file, const size_t len)
{
	int errcode = 0;
	Parser parser = {0};
	parser_init(&parser, arena, file, len);

	for (Node *node = parser_next(&parser);; node = parser_next(&parser)) {
		/* TODO: Work on error recovery */
		if (!node) {
			Span span = parser.span;
			fprintf(stderr, "%s:%zu:%zu:Parse failed:%s\n", filename, span.start_row, span.start_col, parser.error);
			errcode = -1;
			continue;
		} else if (node->kind == N_EOF) {
			break;
		}

		Instruction *instruction = process_node(context, node);
		if (instruction) {
			++context->pc;
			/* Get the offset since the region can reallocate */
			context_push_instruction(context, (Instruction *) ((size_t) instruction - (size_t) arena->region));
		}
	}

	if (errcode) return errcode;
	context->pc = 0;

	// Resolve labels
	for (size_t i = 0; i < context->instruction_len; ++i) {
		/* Set the offset back to the region address */
		context->instructions[i] = (Instruction *) ((size_t) arena->region + (size_t) context->instructions[i]);
		Instruction *instruction = context->instructions[i];
		if (instruction->kind == I_JUMP || instruction->kind == I_JUMPCMP) {
			ssize_t location;
			if (!label_location(context, instruction->data.s, &location)) {
				// TODO: Handle better
				panic("%s:Jump location doesn't exist\n", instruction->data.s);
			}

			instruction->data.offset = location - i - 1;
		}

		if (instruction->kind == I_JUMPPROC) {
			ssize_t location;
			if (!label_location(context, instruction->data.proc.location.s, &location)) {
				// TODO: Handle better
				panic("%s:Jump location doesn't exist\n", instruction->data.s);
			}
			instruction->data.proc.location.offset = location - i - 1;
		}

		if (instruction->kind == I_PPUSH) {
			void *data_ptr;
			if (!resolve_load(context, instruction->data.lit.data.s, &data_ptr)) {
				// TODO: Handle better
				panic("%s:Data name does not exist\n", instruction->data.lit.data.s);
			}
			instruction->data.ptr = data_ptr;
		}
	}

	return errcode;
}

int main(int argc, char **argv)
{
	char *program_name = argv[0];
	const char *path = argv[1];
	struct stat sb = {0};
	if (argc < 2) {
		print_help();
		goto error_1;
	}

	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "%s: cannot find %s: No such file or directory\n", program_name, path);
		goto error_1;
	}
	if (lstat(path, &sb) < 0) {
		fprintf(stderr, "%s: failed to stat %s\n", program_name, path);
		goto error_2;
	}

	Ctx context = {0};
	Arena *arena = arena_create(1024 * 32);
	context_init(&context);

	size_t len = sb.st_size;
	if (parse_src(&context, arena, path, f, len)) goto error_3;
	if (fclose(f)) panic("Failed to close file\n");
	begin_execution(&context);
	context_destroy(&context);
	arena_destroy(arena);

	return 0;

error_3:
	context_destroy(&context);
	arena_destroy(arena);
error_2:
	free(f);
error_1:
	return 1;
}
