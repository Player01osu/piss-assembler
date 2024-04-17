#ifdef TOK

#ifdef TOK_ENUM
TOK(T_COMMA         = ',')
TOK(T_OPEN_BRACKET  = '[')
TOK(T_CLOSE_BRACKET = ']')
TOK(T_EOL           = '\n')
#else
TOK(T_COMMA)
TOK(T_OPEN_BRACKET)
TOK(T_CLOSE_BRACKET)
TOK(T_EOL)
#endif

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

#ifdef TOK_STR
#define INSTR(x, str) TOK_STR(T_##x, str)
#include "instructions.h"
TOK_STR(T_SECTION_DATA, ".data")
TOK_STR(T_SECTION_TEXT, ".text")

TOK_STR(T_EXTERN, "extern")

TOK_STR(T_DD, "dd")
TOK_STR(T_DW, "dw")
TOK_STR(T_DB, "db")
#undef INSTR
#endif
