#!/bin/sh

set -xe

CC="clang"
CFLAGS="-Wall -Wextra -Wpedantic -std=c99 -ggdb -O0 -DDEBUG"
#CFLAGS="-Wall -Wextra -Wpedantic -std=c99 -O2"

SRC="ass.c lexer.c parser.c"
OUT="ass"

${CC} -o ${OUT} ${SRC} ${CFLAGS}
