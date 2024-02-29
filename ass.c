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
#include "arena.h"
#undef ARENA_IMPLEMENTATION

#include "lexer.h"
#include "parser.h"

#define panic(msg) { fprintf(stderr, "%s:%d:"msg, __FILE__, __LINE__); exit(1); }
#define BUF_SIZE 1024 * 4
#define STACK_SIZE 1024 * 16
#define LOCAL_SIZE 256

const char *HELP = "Usage: ass [options] file ...\n";

enum InstructionKind {
	I_POP8,
	I_POP32,
	I_POP64,

	I_IPUSH,
	I_IADD,
	I_ISUB,
	I_IMULT,
	I_IDIV,
	I_IMOD,
	I_IPRINT,

	I_FPUSH,
	I_FADD,
	I_FSUB,
	I_FMULT,
	I_FDIV,
	I_FPRINT,

	I_CPUSH,
	I_CADD,
	I_CSUB,
	I_CMULT,
	I_CDIV,
	I_CMOD,
	I_CPRINT,
	I_CIPRINT,

	I_JUMP,
	I_JUMPCMP,
	I_JUMPPROC,

	I_DUPE8,
	I_DUPE32,
	I_DUPE64,

	I_SWAP8,
	I_SWAP32,
	I_SWAP64,

	I_COPY8,
	I_COPY32,
	I_COPY64,

	I_STORE8,
	I_STORE32,
	I_STORE64,

	I_LOAD8,
	I_LOAD32,
	I_LOAD64,

	I_RET8,
	I_RET32,
	I_RET64,

	I_RET,

	/* Comparison instructions */
	I_ICLT,
	I_ICLE,
	I_ICEQ,
	I_ICGT,
	I_ICGE,

	I_FCLT,
	I_FCLE,
	I_FCEQ,
	I_FCGT,
	I_FCGE,

	I_CCLT,
	I_CCLE,
	I_CCEQ,
	I_CCGT,
	I_CCGE,
};

typedef unsigned char byte;

typedef struct Instruction {
	enum InstructionKind kind;
	union NodeData data;
} Instruction;

typedef struct FramePointer {
	byte *ptr;
	byte *return_stack_ptr;
	byte *start;
	byte locals[LOCAL_SIZE];
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
	byte stack[STACK_SIZE];
	byte return_stack[STACK_SIZE];
	FramePointer *frame_ptr;
	size_t pc;

	LabelMap label_map;

	Instruction *instructions;
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
	context->frame_ptr = context->frame_ptr->prev;
	free(p);
	return context->frame_ptr;
}

bool process_node(Ctx *context, Node *node, Instruction *instruction)
{
	switch (node->kind) {
		case N_POP8: {
			instruction->kind = I_POP8;
		} break;
		case N_POP32: {
			instruction->kind = I_POP32;
		} break;
		case N_POP64: {
			instruction->kind = I_POP64;
		} break;

		case N_IPUSH: {
			instruction->kind = I_IPUSH;
			instruction->data = node->data;
		} break;
		case N_IADD: {
			instruction->kind = I_IADD;
		} break;
		case N_ISUB: {
			instruction->kind = I_ISUB;
		} break;
		case N_IMULT: {
			instruction->kind = I_IMULT;
		} break;
		case N_IDIV: {
			instruction->kind = I_IDIV;
		} break;
		case N_IMOD: {
			instruction->kind = I_IMOD;
		} break;
		case N_IPRINT: {
			instruction->kind = I_IPRINT;
		} break;

		case N_FPUSH: {
			instruction->kind = I_FPUSH;
			instruction->data = node->data;
		} break;
		case N_FADD: {
			instruction->kind = I_FADD;
		} break;
		case N_FSUB: {
			instruction->kind = I_FSUB;
		} break;
		case N_FMULT: {
			instruction->kind = I_FMULT;
		} break;
		case N_FDIV: {
			instruction->kind = I_FDIV;
		} break;
		case N_FPRINT: {
			instruction->kind = I_FPRINT;
		} break;

		case N_CPUSH: {
			instruction->kind = I_CPUSH;
			instruction->data = node->data;
		} break;
		case N_CADD: {
			instruction->kind = I_CADD;
		} break;
		case N_CSUB: {
			instruction->kind = I_CSUB;
		} break;
		case N_CMULT: {
			instruction->kind = I_CMULT;
		} break;
		case N_CDIV: {
			instruction->kind = I_CDIV;
		} break;
		case N_CMOD: {
			instruction->kind = I_CMOD;
		} break;
		case N_CPRINT: {
			instruction->kind = I_CPRINT;
		} break;
		case N_CIPRINT: {
			instruction->kind = I_CIPRINT;
		} break;

		case N_JUMP: {
			instruction->kind = I_JUMP;
			instruction->data = node->data;
		} break;
		case N_JUMPCMP: {
			instruction->kind = I_JUMPCMP;
			instruction->data = node->data;
		} break;
		case N_JUMPPROC: {
			instruction->kind = I_JUMPPROC;
			instruction->data = node->data;
		} break;

		case N_COPY8: {
			instruction->kind = I_COPY8;
			instruction->data = node->data;
		} break;
		case N_COPY32: {
			instruction->kind = I_COPY32;
			instruction->data = node->data;
		} break;
		case N_COPY64: {
			instruction->kind = I_COPY64;
			instruction->data = node->data;
		} break;

		case N_DUPE8: {
			instruction->kind = I_DUPE8;
		} break;
		case N_DUPE32: {
			instruction->kind = I_DUPE32;
		} break;
		case N_DUPE64: {
			instruction->kind = I_DUPE64;
		} break;

		case N_SWAP8: {
			instruction->kind = I_SWAP8;
		} break;
		case N_SWAP32: {
			instruction->kind = I_SWAP32;
		} break;
		case N_SWAP64: {
			instruction->kind = I_SWAP64;
		} break;

		case N_STORE8: {
			instruction->kind = I_STORE8;
			instruction->data = node->data;
		} break;
		case N_STORE32: {
			instruction->kind = I_STORE32;
			instruction->data = node->data;
		} break;
		case N_STORE64: {
			instruction->kind = I_STORE64;
			instruction->data = node->data;
		} break;

		case N_LOAD8: {
			instruction->kind = I_LOAD8;
			instruction->data = node->data;
		} break;
		case N_LOAD32: {
			instruction->kind = I_LOAD32;
			instruction->data = node->data;
		} break;
		case N_LOAD64: {
			instruction->kind = I_LOAD64;
			instruction->data = node->data;
		} break;

		case N_RET8: {
			instruction->kind = I_RET8;
		} break;
		case N_RET32: {
			instruction->kind = I_RET32;
		} break;
		case N_RET64: {
			instruction->kind = I_RET64;
		} break;

		case N_RET: {
			instruction->kind = I_RET;
			instruction->data = node->data;
		} break;

		case N_ICLT: {
			instruction->kind = I_ICLT;
		} break;
		case N_ICLE: {
			instruction->kind = I_ICLE;
		} break;
		case N_ICEQ: {
			instruction->kind = I_ICEQ;
		} break;
		case N_ICGT: {
			instruction->kind = I_ICGT;
		} break;
		case N_ICGE: {
			instruction->kind = I_ICGE;
		} break;

		case N_FCLT: {
			instruction->kind = I_FCLT;
		} break;
		case N_FCLE: {
			instruction->kind = I_FCLE;
		} break;
		case N_FCEQ: {
			instruction->kind = I_FCEQ;
		} break;
		case N_FCGT: {
			instruction->kind = I_FCGT;
		} break;
		case N_FCGE: {
			instruction->kind = I_FCGE;
		} break;

		case N_CCLT: {
			instruction->kind = I_CCLT;
		} break;
		case N_CCLE: {
			instruction->kind = I_CCLE;
		} break;
		case N_CCEQ: {
			instruction->kind = I_CCEQ;
		} break;
		case N_CCGT: {
			instruction->kind = I_CCGT;
		} break;
		case N_CCGE: {
			instruction->kind = I_CCGE;
		} break;

		case N_LABEL: {
			if (!insert_label(&context->label_map, node->data.s, context->pc)) {
				panic("Failed to create label");
			}
			return false;
		} break;
		default: {
			panic("Unimplemented");
		}
	}

	return true;
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

byte *pop_stack(Ctx *context, size_t n)
{
	context->frame_ptr->ptr -= n;
	byte *top = context->frame_ptr->ptr;
	return top;
}

void exec_instruction(Ctx *context, Instruction *instruction)
{
#define STACK_CHECK if (empty_stack(context)) goto empty_stack;
	switch (instruction->kind) {
		case I_CPUSH: {
			char item = (char) instruction->data.lit.data.i;
			push_stack(context, &item, sizeof(char));
		} break;
		case I_CPRINT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			char *a = (char *)(&stack_ptr[-sizeof(char)]);
			printf("%c", *a);
		} break;
		case I_CIPRINT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			char *a = (char *)(&stack_ptr[-sizeof(char)]);
			printf("%d", *a);
		} break;

		case I_IPUSH: {
			int item = instruction->data.lit.data.i;
			push_stack(context, &item, sizeof(int));
		} break;
		case I_IADD: {
			STACK_CHECK;
			int *b = (int *)pop_stack(context, 4);
			STACK_CHECK;
			int *a = (int *)pop_stack(context, 4);

			int item = *a + *b;
			push_stack(context, &item, sizeof(int));
		} break;
		case I_ISUB: {
			STACK_CHECK;
			int *b = (int *)pop_stack(context, 4);
			STACK_CHECK;
			int *a = (int *)pop_stack(context, 4);

			int item = *a - *b;
			push_stack(context, &item, sizeof(int));
		} break;
		case I_IMULT: {
			STACK_CHECK;
			int *b = (int *)pop_stack(context, 4);
			STACK_CHECK;
			int *a = (int *)pop_stack(context, 4);

			int item = *a * *b;
			push_stack(context, &item, sizeof(int));
		} break;
		case I_IDIV: {
			STACK_CHECK;
			int *b = (int *)pop_stack(context, 4);
			STACK_CHECK;
			int *a = (int *)pop_stack(context, 4);

			int item = *a / *b;
			push_stack(context, &item, sizeof(int));
		} break;
		case I_IMOD: {
			STACK_CHECK;
			int *b = (int *)pop_stack(context, 4);
			STACK_CHECK;
			int *a = (int *)pop_stack(context, 4);

			int item = *a % *b;
			push_stack(context, &item, sizeof(int));
		} break;
		case I_IPRINT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			int *a = (int *)(&stack_ptr[-sizeof(int)]);
			printf("%d", *a);
		} break;
		case I_ICEQ: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			int *b = (int *)(&stack_ptr[-(sizeof(int) * 1)]);
			int *a = (int *)(&stack_ptr[-(sizeof(int) * 2)]);
			bool item = *a == *b;
			push_stack(context, &item, 1);
		} break;
		case I_ICLT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			int *b = (int *)&stack_ptr[-(sizeof(int) * 1)];
			int *a = (int *)&stack_ptr[-(sizeof(int) * 2)];
			bool item = *a < *b;
			push_stack(context, &item, 1);
		} break;
		case I_ICLE: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			int *b = (int *)&stack_ptr[-(sizeof(int) * 1)];
			int *a = (int *)&stack_ptr[-(sizeof(int) * 2)];
			bool item = *a <= *b;
			push_stack(context, &item, 1);
		} break;
		case I_ICGT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			int *b = (int *)&stack_ptr[-(sizeof(int) * 1)];
			int *a = (int *)&stack_ptr[-(sizeof(int) * 2)];
			bool item = *a > *b;
			push_stack(context, &item, 1);
		} break;
		case I_ICGE: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			int *b = (int *)&stack_ptr[-(sizeof(int) * 1)];
			int *a = (int *)&stack_ptr[-(sizeof(int) * 2)];
			bool item = *a >= *b;
			push_stack(context, &item, 1);
		} break;

		case I_FPUSH: {
			float item = instruction->data.lit.data.f;
			push_stack(context, &item, sizeof(float));
		} break;
		case I_FADD: {
			STACK_CHECK;
			float *b = (float *)pop_stack(context, 4);
			STACK_CHECK;
			float *a = (float *)pop_stack(context, 4);

			float item = *a + *b;
			push_stack(context, &item, sizeof(float));
		} break;
		case I_FSUB: {
			STACK_CHECK;
			float *b = (float *)pop_stack(context, 4);
			STACK_CHECK;
			float *a = (float *)pop_stack(context, 4);

			float item = *a - *b;
			push_stack(context, &item, sizeof(float));
		} break;
		case I_FMULT: {
			STACK_CHECK;
			float *b = (float *)pop_stack(context, 4);
			STACK_CHECK;
			float *a = (float *)pop_stack(context, 4);

			float item = *a * *b;
			push_stack(context, &item, sizeof(float));
		} break;
		case I_FDIV: {
			STACK_CHECK;
			float *b = (float *)pop_stack(context, 4);
			STACK_CHECK;
			float *a = (float *)pop_stack(context, 4);

			float item = *a / *b;
			push_stack(context, &item, sizeof(float));
		} break;
		case I_FPRINT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			float *a = (float *)(&stack_ptr[-sizeof(float)]);
			printf("%f", *a);
		} break;
		case I_FCEQ: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			float *b = (float *)(&stack_ptr[-(sizeof(float) * 1)]);
			float *a = (float *)(&stack_ptr[-(sizeof(float) * 2)]);
			bool item = *a == *b;
			push_stack(context, &item, 1);
		} break;
		case I_FCLT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			float *b = (float *)&stack_ptr[-(sizeof(float) * 1)];
			float *a = (float *)&stack_ptr[-(sizeof(float) * 2)];
			bool item = *a < *b;
			push_stack(context, &item, 1);
		} break;
		case I_FCLE: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			float *b = (float *)&stack_ptr[-(sizeof(float) * 1)];
			float *a = (float *)&stack_ptr[-(sizeof(float) * 2)];
			bool item = *a <= *b;
			push_stack(context, &item, 1);
		} break;
		case I_FCGT: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			float *b = (float *)&stack_ptr[-(sizeof(float) * 1)];
			float *a = (float *)&stack_ptr[-(sizeof(float) * 2)];
			bool item = *a > *b;
			push_stack(context, &item, 1);
		} break;
		case I_FCGE: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			float *b = (float *)&stack_ptr[-(sizeof(float) * 1)];
			float *a = (float *)&stack_ptr[-(sizeof(float) * 2)];
			bool item = *a >= *b;
			push_stack(context, &item, 1);
		} break;

		case I_POP8: {
			STACK_CHECK;
			pop_stack(context, 1);
		} break;
		case I_POP32: {
			STACK_CHECK;
			pop_stack(context, 4);
		} break;
		case I_POP64: {
			STACK_CHECK;
			pop_stack(context, 8);
		} break;
		case I_SWAP8: {
			STACK_CHECK;
			byte *a = pop_stack(context, 1);
			byte *b = pop_stack(context, 1);
			byte tmp[1] = {0};
			memcpy(tmp, b, 1);
			push_stack(context, a, 1);
			push_stack(context, tmp, 1);
		} break;
		case I_SWAP32: {
			STACK_CHECK;
			byte *a = pop_stack(context, 4);
			byte *b = pop_stack(context, 4);
			byte tmp[4] = {0};
			memcpy(tmp, b, 4);
			push_stack(context, a, 4);
			push_stack(context, tmp, 4);
		} break;
		case I_SWAP64: {
			STACK_CHECK;
			byte *a = pop_stack(context, 8);
			byte *b = pop_stack(context, 8);
			byte tmp[8] = {0};
			memcpy(tmp, b, 8);
			push_stack(context, a, 8);
			push_stack(context, tmp, 8);
		} break;
		case I_DUPE8: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			byte *a = &stack_ptr[-1];
			push_stack(context, a, 1);
		} break;
		case I_DUPE32: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			byte *a = &stack_ptr[-4];
			push_stack(context, a, 4);
		} break;
		case I_DUPE64: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			byte *a = &stack_ptr[-8];
			push_stack(context, a, 8);
		} break;
		case I_COPY8: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			byte *a = &stack_ptr[-1];
			size_t n = instruction->data.n;
			for (size_t i = 0; i < n; ++i) {
				push_stack(context, a, 1);
			}
		} break;
		case I_COPY32: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			byte *a = &stack_ptr[-4];
			size_t n = instruction->data.n;
			for (size_t i = 0; i < n; ++i) {
				push_stack(context, a, 4);
			}
		} break;
		case I_COPY64: {
			STACK_CHECK;
			byte *stack_ptr = context->frame_ptr->ptr;
			byte *a = &stack_ptr[-8];
			size_t n = instruction->data.n;
			for (size_t i = 0; i < n; ++i) {
				push_stack(context, a, 8);
			}
		} break;
		case I_STORE8: {
			STACK_CHECK;
			size_t n = instruction->data.n;
			byte *locals = context->frame_ptr->locals;
			byte *slot = &locals[n];
			byte *a = pop_stack(context, 1);
			memcpy(slot, a, 1);
		} break;
		case I_STORE32: {
			STACK_CHECK;
			size_t n = instruction->data.n;
			byte *locals = context->frame_ptr->locals;
			byte *slot = &locals[n];
			byte *a = pop_stack(context, 4);
			memcpy(slot, a, 4);
		} break;
		case I_STORE64: {
			STACK_CHECK;
			size_t n = instruction->data.n;
			byte *locals = context->frame_ptr->locals;
			byte *slot = &locals[n];
			byte *a = pop_stack(context, 8);
			memcpy(slot, a, 8);
		} break;
		case I_LOAD8: {
			size_t n = instruction->data.n;
			byte *locals = context->frame_ptr->locals;
			byte *slot = &locals[n];
			push_stack(context, slot, 1);
		} break;
		case I_LOAD32: {
			size_t n = instruction->data.n;
			byte *locals = context->frame_ptr->locals;
			byte *slot = &locals[n];
			push_stack(context, slot, 4);
		} break;
		case I_LOAD64: {
			size_t n = instruction->data.n;
			byte *locals = context->frame_ptr->locals;
			byte *slot = &locals[n];
			push_stack(context, slot, 8);
		} break;
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
		} break;
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
		} break;
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
		} break;
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
		} break;
		case I_JUMPPROC: {
			size_t argc = instruction->data.proc.argc;
			FramePointer *stack_ptr = context->frame_ptr;
			FramePointer *stack_ptr_new = malloc(sizeof(FramePointer));
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
		} break;
		case I_JUMP: {
			context->pc += instruction->data.offset;
		} break;
		case I_JUMPCMP: {
			STACK_CHECK;
			byte *ptr = &context->frame_ptr->ptr[-1];
			if (*ptr) {
				context->pc += instruction->data.offset;
				break;
			}
		} break;
		default: {
			fprintf(stderr, "%s:%d:UNIMPLMEMENTED:%d\n", __FILE__, __LINE__, instruction->kind);
		}
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

	context->instructions = malloc(sizeof(*context->instructions) * 16);
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
	free(context->instructions);
}

void context_push_instruction(Ctx *context, Instruction instruction)
{
	if (context->instruction_len >= context->instruction_cap) {
		context->instruction_cap *= 2;
		context->instructions =
			realloc(context->instructions, sizeof(*context->instructions) * context->instruction_cap);
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
		exec_instruction(context, &context->instructions[context->pc++]);
	}
}

void parse_src(Ctx *context, Arena *arena, char *src, size_t len)
{
	Parser parser = {0};
	parser_init(&parser, arena, src, len);

	for (Node *node = parser_next(&parser); node->kind != N_EOF; node = parser_next(&parser)) {
		if (node->kind == N_ILLEGAL) {
			printf("Parse failed:%s\n", parser.error);
			break;
		}

		Instruction instruction = {0};
		if (process_node(context, node, &instruction)) {
			++context->pc;
			context_push_instruction(context, instruction);
		}
	}
	context->pc = 0;

	// Resolve labels
	for (size_t i = 0; i < context->instruction_len; ++i) {
		Instruction *instruction = &context->instructions[i];
		if (instruction->kind == I_JUMP || instruction->kind == I_JUMPCMP) {
			ssize_t location;
			if (!label_location(context, instruction->data.s, &location)) {
				// TODO: Handle better
				printf("%s\n", instruction->data.s);
				panic("Jump location doesn't exist\n");
			}

			instruction->data.offset = location - i - 1;
		}

		if (instruction->kind == I_JUMPPROC) {
			ssize_t location;
			if (!label_location(context, instruction->data.proc.location.s, &location)) {
				// TODO: Handle better
				printf("%s\n", instruction->data.s);
				panic("Jump location doesn't exist\n");
			}
			instruction->data.proc.location.offset = location - i - 1;
		}
	}
}

int main(int argc, char **argv)
{
	char *program_name = argv[0];
	const char *path = argv[1];
	struct stat sb = {0};
	if (argc < 2) {
		print_help();
		return 1;
	}

	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "%s: cannot find %s: No such file or directory\n", program_name, path);
		return 1;
	}
	if (lstat(path, &sb) < 0) {
		fprintf(stderr, "%s: failed to stat %s\n", program_name, path);
		return 1;
	}
	char s[sb.st_size]; // This is NOT null terminated
	size_t len = fread(s, sizeof(char), sb.st_size, f);
	if (fclose(f)) panic("Failed to close file\n");
	assert(len == (size_t)sb.st_size);

	Ctx context = {0};
	Arena *arena = arena_create(1024 * 16);
	context_init(&context);

	parse_src(&context, arena, s, len);
	begin_execution(&context);

	return 0;
}
