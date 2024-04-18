#ifdef TOK

#ifdef TOK_ENUM
#define X_ENUM(x) x
#else
#define X_ENUM(_)
#endif
TOK(T_COMMA         X_ENUM(= ','))
TOK(T_OPEN_BRACKET  X_ENUM(= '['))
TOK(T_CLOSE_BRACKET X_ENUM(= ']'))
TOK(T_EOL           X_ENUM(= '\n'))
#undef X_ENUM

#define INSTR(x, _) TOK(T_##x)
#include "instructions.h"
#undef INSTR

TOK(T_DATATYPE)
TOK(T_LABEL)

TOK(T_SECTION_DATA)
TOK(T_SECTION_TEXT)

TOK(T_IDENT)
TOK(T_UINUMLIT) // Unsigned int literal (8-bytes)
TOK(T_INUMLIT)  // Signed int literal   (8-bytes)
TOK(T_FNUMLIT)  // Double               (8-bytes)
TOK(T_SLIT)     // String literal

TOK(T_DD)
TOK(T_DW)
TOK(T_DB)
TOK(T_EXTERN)
TOK(T_EOF)

TOK(T_ILLEGAL)

#endif

#ifdef TOK_KW
#define INSTR(x, str) TOK_KW(T_##x, str)
#include "instructions.h"
#undef INSTR

TOK_KW(T_SECTION_DATA, ".data")
TOK_KW(T_SECTION_TEXT, ".text")

TOK_KW(T_EXTERN, "extern")

TOK_KW(T_DD, "dd")
TOK_KW(T_DW, "dw")
TOK_KW(T_DB, "db")
#endif
