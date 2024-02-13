#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <sys/types.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"
#undef ARENA_IMPLEMENTATION

#include "lexer.h"
#include "parser.h"

#define panic(msg) { fprintf(stderr, "%s:%d:"msg, __FILE__, __LINE__); exit(1); }
#define BUF_SIZE 1024 * 4
#define STACK_SIZE 1024 * 16

const char *HELP = "Usage: ass [options] file ...\n";

typedef struct Object {
	void *ptr;
	size_t size;
	size_t ref;
} Object;

enum InstructionKind {
	I_PUSH,
	I_POP,

	I_PRINT,

	I_ADD,
	I_SUB,
	I_MULT,
	I_DIV,

	I_JUMP,
	I_JUMPCMP,

	I_COPY,
	I_DUPE,
	I_SWAP,

	I_PUSHFRAME,
	I_POPFRAME,
	I_STORE,
	I_STORETOP,
	I_LOAD,

	/* Comparison instructions */
	I_CLT,
	I_CLE,
	I_CEQ,
	I_CGE,
	I_CGT,
};

typedef struct Instruction {
	enum InstructionKind kind;
	union NodeData data;
} Instruction;

typedef struct FramePointer {
	Object *ptr;
	Object *start;
	struct FramePointer *prev;
} FramePointer;

typedef struct Label {
	const char *name;
	size_t location;
} Label;

typedef struct LabelMap {
	Label *labels;
	size_t cap;
	size_t len;
} LabelMap;

typedef struct Ctx {
	Object stack[STACK_SIZE];
	FramePointer *frame_ptr;
	ssize_t return_offset;
	size_t pc;

	LabelMap label_map;

	Instruction **instructions;
	size_t instruction_cap;
	size_t instruction_len;
} Ctx;

void print_help(void)
{
	fputs(HELP, stdout);
}

bool insert_label(LabelMap *label_map, const char *name, size_t location)
{
	if (label_map->len >= label_map->cap) {
		label_map->cap *= 2;
		label_map->labels =
			realloc(label_map->labels, sizeof(Label) * label_map->cap);
	}
	label_map->labels[label_map->len++] = (Label){
		.name = name,
		.location = location,
	};
	return true;
}

FramePointer *context_push_frame(Ctx *context)
{
	FramePointer *p = malloc(sizeof(FramePointer));
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
	Object *start = p->start;
	Object *end = p->ptr;
	while (end != start) {
		// Drop any objects that have 1 or 0 references
		if (end->ref <= 1) {
			free(end->ptr);
		}
		end -= 1;
	}
	context->frame_ptr = context->frame_ptr->prev;
	free(p);
	return context->frame_ptr;
}

Instruction *process_node(Ctx *context, Node *node)
{
	Instruction instruction = {0};

	switch (node->kind) {
		case N_PUSH: {
			instruction.kind = I_PUSH;
			instruction.data = node->data;
		} break;
		case N_POP: {
			instruction.kind = I_POP;
		} break;
		case N_PRINT: {
			instruction.kind = I_PRINT;
			instruction.data = node->data;
		} break;
		case N_ADD: {
			instruction.kind = I_ADD;
		} break;
		case N_SUB: {
			instruction.kind = I_SUB;
		} break;
		case N_MULT: {
			instruction.kind = I_MULT;
		} break;
		case N_DIV: {
			instruction.kind = I_DIV;
		} break;
		case N_JUMP: {
			instruction.kind = I_JUMP;
			instruction.data = node->data;
		} break;
		case N_JUMPCMP: {
			instruction.kind = I_JUMPCMP;
			instruction.data = node->data;
		} break;
		case N_COPY: {
			instruction.kind = I_COPY;
			instruction.data = node->data;
		} break;
		case N_DUPE: {
			instruction.kind = I_DUPE;
		} break;
		case N_SWAP: {
			instruction.kind = I_SWAP;
		} break;
		case N_PUSHFRAME: {
			instruction.kind = I_PUSHFRAME;
		} break;
		case N_POPFRAME: {
			instruction.kind = I_POPFRAME;
		} break;
		case N_STORE: {
			instruction.kind = I_STORE;
			instruction.data = node->data;
		} break;
		case N_STORETOP: {
			instruction.kind = I_STORETOP;
			instruction.data = node->data;
		} break;
		case N_LOAD: {
			instruction.kind = I_LOAD;
			instruction.data = node->data;
		} break;
		case N_CLT: {
			instruction.kind = I_CLT;
		} break;
		case N_CLE: {
			instruction.kind = I_CLE;
		} break;
		case N_CEQ: {
			instruction.kind = I_CEQ;
		} break;
		case N_CGE: {
			instruction.kind = I_CGE;
		} break;
		case N_CGT: {
			instruction.kind = I_CGT;
		} break;
		case N_LABEL: {
			if (!insert_label(&context->label_map, node->data.s, context->pc)) {
				panic("Failed to create label");
			}
			return NULL;
		} break;
		default: {
			panic("Unimplemented");
		}
	}

	Instruction *instruction_heap = malloc(sizeof(Instruction));
	memcpy(instruction_heap, &instruction, sizeof(Instruction));
	return instruction_heap;
}

void print_stack(Ctx *context, Ty ty)
{
	Object *stack_ptr = context->frame_ptr->ptr;
	stack_ptr += -1;
	switch (ty.kind) {
		case TY_INT: {
			printf("%d\n", *((int *)stack_ptr->ptr));
		} break;
		case TY_PTR_NAME: {
			printf("%s\n", "UNIMPLEMENTED");
		} break;
	}
}

bool empty_stack(Ctx *context)
{
	return context->frame_ptr->ptr <= context->frame_ptr->start;
}

void push_stack(Ctx *context, void *data, size_t size)
{
	Object *obj = context->frame_ptr->ptr;
	obj->size = size;
	obj->ptr = malloc(size);
	obj->ref = 1;
	memcpy(obj->ptr, data, size);
	context->frame_ptr->ptr += 1;
}

Object *pop_stack(Ctx *context)
{
	// TODO: Cannot trivially free because other things
	// may be referencing this
	//
	// Consider reference counting
	Object *top = --context->frame_ptr->ptr;
	if (top->ref > 0) {
		top->ref -= 1;
	}
	return top;
}

void exec_instruction(Ctx *context, Instruction *instruction)
{
#define STACK_CHECK if (empty_stack(context)) goto empty_stack;
#define SIZE_CHECK(a, b) if (a->size != b->size) goto incompatible_types;

	switch (instruction->kind) {
		case I_ADD: {
			STACK_CHECK;
			Object *b = pop_stack(context);
			STACK_CHECK;
			Object *a = pop_stack(context);
			SIZE_CHECK(a, b);

			int item = *(int *)a->ptr + *(int *)b->ptr;
			size_t size = a->size;
			push_stack(context, &item, size);
		} break;
		case I_SUB: {
			STACK_CHECK;
			Object *b = pop_stack(context);
			STACK_CHECK;
			Object *a = pop_stack(context);

			SIZE_CHECK(a, b);

			if (a->size != b->size) {
				panic("Types not of same size");
			}

			int item = *(int *)a->ptr - *(int *)b->ptr;
			size_t size = a->size;
			push_stack(context, &item, size);
		} break;
		case I_CEQ: {
			STACK_CHECK;
			Object *stack_ptr = context->frame_ptr->ptr;
			Object *b = &stack_ptr[-1];
			Object *a = &stack_ptr[-2];
			SIZE_CHECK(a, b);

			// TODO: Make these char
			int item = 1;
			size_t size = sizeof(int);

			for (size_t i = 0; i < a->size; ++i) {
				if (((char *)a->ptr)[i] != ((char *)b->ptr)[i]) {
					item = 0;
					break;
				}
			}
			push_stack(context, &item, size);
		} break;
		case I_CLT: {
			STACK_CHECK;
			Object *b = pop_stack(context);
			STACK_CHECK;
			Object *a = pop_stack(context);
			SIZE_CHECK(a, b);

			// TODO: Make these char
			int item = 1;
			size_t size = sizeof(int);

			// TODO: Borked
			for (size_t i = 0; i < a->size; ++i) {
				if (!(((char *)a->ptr)[i] < ((char *)b->ptr)[i])) {
					item = 0;
					break;
				}
			}
			push_stack(context, &item, size);
		} break;
		case I_PRINT: {
			STACK_CHECK;
			print_stack(context, instruction->data.ty);
		} break;
		case I_PUSH: {
			int item = instruction->data.sized.data.i;
			size_t size = instruction->data.sized.size;
			push_stack(context, &item, size);
		} break;
		case I_POP: {
			STACK_CHECK;
			pop_stack(context);
		} break;
		case I_SWAP: {
			STACK_CHECK;
			Object *stack_ptr = context->frame_ptr->ptr;

			Object *stack_top = stack_ptr - 1;
			Object *stack_bot = stack_ptr - 2;
			Object *stack_tmp = stack_ptr;

			// Temporary moves top of stack into next position
			memcpy(stack_tmp, stack_top, sizeof(Object));
			memcpy(stack_top, stack_bot, sizeof(Object));
			memcpy(stack_bot, stack_tmp, sizeof(Object));
		} break;
		case I_DUPE: {
			STACK_CHECK;
			Object *obj = context->frame_ptr->ptr;
			memcpy(obj, obj - 1, sizeof(Object));
			context->frame_ptr->ptr += 1;
			obj->ref += 1;
		} break;
		case I_COPY: {
			STACK_CHECK;
			size_t n = instruction->data.n;
			Object *obj = context->frame_ptr->ptr;
			for (size_t i = 0; i < n; ++i) {
				memcpy(obj + i, obj - i, sizeof(Object));
				context->frame_ptr->ptr += 1;
				obj->ref += 1;
			}
		} break;
		case I_PUSHFRAME: {
			context_push_frame(context);
		} break;
		case I_POPFRAME: {
			context_pop_frame(context);
		} break;
		case I_LOAD: {
			STACK_CHECK;
			size_t n = instruction->data.n;
			Object *start = context->frame_ptr->start;
			Object *stack_ptr = context->frame_ptr->ptr;
			Object *obj = &start[n];
			memcpy(stack_ptr, obj, sizeof(Object));
			context->frame_ptr->ptr += 1;
			obj->ref += 1;
		} break;
		case I_STORE: {
			STACK_CHECK;
			size_t n = instruction->data.store.idx;
			Sized sized = instruction->data.store.sized;
			Object *start = context->frame_ptr->start;
			Object *obj = &start[n];
			if (obj->size != sized.size) {
				panic("Objects not of same size\n");
			}
			// Only memcpy the data because they are the same size
			memcpy(obj->ptr, &sized.data.i, obj->size);
		} break;
		case I_STORETOP: {
			STACK_CHECK;
			size_t n = instruction->data.n;
			Object *stack_ptr = context->frame_ptr->ptr;
			// TODO: Uhh what if top of stack is where it stores to
			// and it refers to self
			Object *start = context->frame_ptr->start;
			Object *obj = &start[n];
			Object *top = &stack_ptr[-1];
			SIZE_CHECK(obj, top);
			// Only memcpy the data because they are the same size
			memcpy(obj->ptr, top->ptr, obj->size);
		} break;
		case I_JUMP: {
			context->pc += instruction->data.offset;
		} break;
		case I_JUMPCMP: {
			STACK_CHECK;
			Object *object = &context->frame_ptr->ptr[-1];
			for (size_t i = 0; i < object->size; ++i) {
				if (((char *)object->ptr)[i]) {
					context->pc += instruction->data.offset;
					break;
				}
			}
		} break;
		// call_func
		// set return offset
		// push stackframe

		// return
		// set return object
		// pop stackframe
		// push return object
		// jump return offset
		default: {
			fprintf(stderr, "%s:%d:UNIMPLMEMENTED:%d\n", __FILE__, __LINE__, instruction->kind);
		}
	}



	return;
empty_stack:
	puts("Stack is empty\n");
	return;
incompatible_types:
	panic("Types not of same size");
	return;

#undef STACK_CHECK
#undef SIZE_CHECK
}

void context_init(Ctx *context)
{
	context_push_frame(context);
	context->frame_ptr->ptr = context->stack;
	context->frame_ptr->start = context->stack;
	context->frame_ptr->prev = NULL;

	context->instructions = malloc(sizeof(Node *) * 16);
	context->instruction_cap = 16;
	context->instruction_len = 0;

	context->label_map = (LabelMap){
		.labels = malloc(sizeof(Label) * 16),
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
	for (size_t i = 0; i < context->instruction_len; ++i) {
		free(context->instructions[i]);
	}
	free(context->instructions);
}

void context_push_instruction(Ctx *context, Instruction *instruction)
{
	if (context->instruction_len >= context->instruction_cap) {
		context->instruction_cap *= 2;
		context->instructions =
			realloc(context->instructions, sizeof(Node *) * context->instruction_cap);
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

void begin_execution(Ctx *context)
{
	while (context->pc < context->instruction_len) {
		exec_instruction(context, context->instructions[context->pc++]);
	}
}

void parse_src(Ctx *context, Arena *arena, char *src, size_t len)
{
	Parser parser = {0};
	parser_init(&parser, arena, src, len);

	for (Node *node = parser_next(&parser); node->kind != N_EOF; node = parser_next(&parser)) {
		if (node->kind == N_ILLEGAL) {
			printf("%s\n", parser.error);
			break;
		}

		Instruction *instruction = process_node(context, node);
		if (instruction) {
			++context->pc;
			context_push_instruction(context, instruction);
		}
	}
	context->pc = 0;

	// Resolve labels
	for (size_t i = 0; i < context->instruction_len; ++i) {
		Instruction *instruction = context->instructions[i];
		if (instruction->kind == I_JUMP || instruction->kind == I_JUMPCMP) {
			ssize_t location;
			if (!label_location(context, instruction->data.s, &location)) {
				// TODO: Handle better
				printf("%s\n", instruction->data.s);
				panic("Jump location doesn't exist\n");
			}

			instruction->data.offset = location - i - 1;
		}
	}

}

void compile_test(void)
{
	char s[BUF_SIZE] = {0};
	const char *path = "./examples/for_loop2.piss";
	FILE *f = fopen(path, "rb");
	size_t len = fread(s, sizeof(char), BUF_SIZE, f);
	if (fclose(f)) panic("Failed to close file\n");

	Ctx context = {0};
	Arena *arena = arena_create(1024 * 16);
	context_init(&context);

	parse_src(&context, arena, s, len);
	begin_execution(&context);

	arena_destroy(arena);
}

int main(int argc, char **argv)
{
	char *program_name = argv[0];
	if (argc < 2) {
		print_help();
		return 1;
	}

	char s[BUF_SIZE] = {0};
	const char *path = argv[1];
	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "%s: cannot find %s: No such file or directory\n", program_name, path);
		return 1;
	}
	size_t len = fread(s, sizeof(char), BUF_SIZE, f);
	if (fclose(f)) panic("Failed to close file\n");

	Ctx context = {0};
	Arena *arena = arena_create(1024 * 16);
	context_init(&context);

	parse_src(&context, arena, s, len);
	begin_execution(&context);

	return 0;
}
