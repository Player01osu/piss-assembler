/*** Instructions ***/
INSTR(POP8,     "pop8")
INSTR(POP32,    "pop32")
INSTR(POP64,    "pop64")

INSTR(ULPUSH,   "ulpush")
INSTR(ULADD,    "uladd")
INSTR(ULSUB,    "ulsub")
INSTR(ULMULT,   "ulmult")
INSTR(ULDIV,    "uldiv")
INSTR(ULMOD,    "ulmod")
INSTR(ULPRINT,  "ulprint")

INSTR(IPUSH,    "ipush")
INSTR(IADD,     "iadd")
INSTR(ISUB,     "isub")
INSTR(IMULT,    "imult")
INSTR(IDIV,     "idiv")
INSTR(IMOD,     "imod")
INSTR(IPRINT,   "iprint")

INSTR(FPUSH,    "fpush")
INSTR(FADD,     "fadd")
INSTR(FSUB,     "fsub")
INSTR(FMULT,    "fmult")
INSTR(FDIV,     "fdiv")
INSTR(FPRINT,   "fprint")

INSTR(CPUSH,    "cpush")
INSTR(CADD,     "cadd")
INSTR(CSUB,     "csub")
INSTR(CMULT,    "cmult")
INSTR(CDIV,     "cdiv")
INSTR(CMOD,     "cmod")
INSTR(CPRINT,   "cprint")
INSTR(CIPRINT,  "ciprint")

INSTR(PPUSH,    "ppush")
INSTR(PLOAD,    "pload")
INSTR(PDEREF8,  "pderef8")
INSTR(PDEREF32, "pderef32")
INSTR(PDEREF64, "pderef64")
INSTR(PDEREF,   "pderef")
INSTR(PSET8,    "pset8")
INSTR(PSET32,   "pset32")
INSTR(PSET64,   "pset64")
INSTR(PSET,     "pset")

INSTR(JUMP,     "jump")
INSTR(JUMPCMP,  "jumpcmp")
INSTR(JUMPPROC, "jumpproc")

INSTR(DUPE8,    "dupe8")
INSTR(DUPE32,   "dupe32")
INSTR(DUPE64,   "dupe64")

INSTR(SWAP8,    "swap8")
INSTR(SWAP32,   "swap32")
INSTR(SWAP64,   "swap64")

INSTR(COPY8,    "copy8")
INSTR(COPY32,   "copy32")
INSTR(COPY64,   "copy64")

INSTR(STORE8,   "store8")
INSTR(STORE32,  "store32")
INSTR(STORE64,  "store64")

INSTR(LOAD8,    "load8")
INSTR(LOAD32,   "load32")
INSTR(LOAD64,   "load64")

INSTR(RET8,     "ret8")
INSTR(RET32,    "ret32")
INSTR(RET64,    "ret64")

INSTR(RET,      "ret")

/* Comparison instructions */
INSTR(ICLT,     "iclt")
INSTR(ICLE,     "icle")
INSTR(ICEQ,     "iceq")
INSTR(ICGT,     "icgt")
INSTR(ICGE,     "icge")

INSTR(ULCLT,    "ulclt")
INSTR(ULCLE,    "ulcle")
INSTR(ULCEQ,    "ulceq")
INSTR(ULCGT,    "ulcgt")
INSTR(ULCGE,    "ulcge")

INSTR(FCLT,     "fclt")
INSTR(FCLE,     "fcle")
INSTR(FCEQ,     "fceq")
INSTR(FCGT,     "fcgt")
INSTR(FCGE,     "fcge")

INSTR(CCLT,     "cclt")
INSTR(CCLE,     "ccle")
INSTR(CCEQ,     "cceq")
INSTR(CCGT,     "ccgt")
INSTR(CCGE,     "ccge")
