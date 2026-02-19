/*
 * mipsu — MIPS32 utilities
 *
 * A small UNIX-style toolkit for decoding, encoding, disassembling,
 * and inspecting MIPS32 instructions.
 *
 * Intended primarily for educational and experimental use.
 *
 * Author: mxwll013 (https://github.com/mxwll013)
 * License: MIT
 *
 * Copyright (c) 2026 mxwll013
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIPSU_VERSION "1.0.1"

#define true  1
#define false 0

typedef int  bool_t;
typedef FILE file_t;

typedef uint32_t               mipsu_word_t;
typedef enum mipsu_type        mipsu_type_t;
typedef enum mipsu_op_fmt      mipsu_op_fmt_t;
typedef struct mipsu_op_entry  mipsu_op_entry_t;
typedef struct mipsu_reg_entry mipsu_reg_entry_t;
typedef struct mipsu_field     mipsu_field_t;

typedef enum mipsu_exit         mipsu_exit_t;
typedef enum mipsu_result       mipsu_result_t;
typedef enum mipsu_flag         mipsu_flag_t;
typedef struct mipsu_flag_entry mipsu_flag_entry_t;
typedef struct mipsu_cmd        mipsu_cmd_t;
typedef struct mipsu_ctx        mipsu_ctx_t;

enum mipsu_type {
    MIPSU_TYPE_R,
    MIPSU_TYPE_I,
    MIPSU_TYPE_J,
};

enum mipsu_op_fmt {
    MIPSU_OP_FMT_UNKNOWN,
    MIPSU_OP_FMT_NONE,

    MIPSU_OP_FMT_RS,
    MIPSU_OP_FMT_RD,
    MIPSU_OP_FMT_RS_RT,
    MIPSU_OP_FMT_RD_RS_RT,
    MIPSU_OP_FMT_RD_RT_RS,
    MIPSU_OP_FMT_RD_RT_SH,

    MIPSU_OP_FMT_RS_IMM,
    MIPSU_OP_FMT_RT_IMM,
    MIPSU_OP_FMT_RT_IMM_RS,
    MIPSU_OP_FMT_RT_RS_IMM,
    MIPSU_OP_FMT_RS_RT_IMM,

    MIPSU_OP_FMT_ADDR,
};

enum mipsu_exit {
    MIPSU_EXIT_OK,

    MIPSU_EXIT_USAGE,
    MIPSU_EXIT_PARSE,
    MIPSU_EXIT_INTERNAL,
};

enum mipsu_result {
    MIPSU_RESULT_OK,

    MIPSU_RESULT_BAD_CMD,
    MIPSU_RESULT_BAD_ARGC,
    MIPSU_RESULT_MISSING_ARGS,
    MIPSU_RESULT_TOO_MANY_ARGS,
    MIPSU_RESULT_FROM_FILE,
    MIPSU_RESULT_RAW_STDIN,
    MIPSU_RESULT_RAW_STDOUT,
    MIPSU_RESULT_STDIN_CHAR,

    MIPSU_RESULT_BAD_DEC,
    MIPSU_RESULT_BAD_RADIX,
    MIPSU_RESULT_BAD_HEX,
    MIPSU_RESULT_MISSING_HEXITS,
    MIPSU_RESULT_TOO_MANY_HEXITS,
    MIPSU_RESULT_BAD_BIN,
    MIPSU_RESULT_MISSING_BITS,
    MIPSU_RESULT_TOO_MANY_BITS,
    MIPSU_RESULT_BAD_OP,
    MIPSU_RESULT_BAD_OP_FMT,
    MIPSU_RESULT_BAD_OP_TYPE,
    MIPSU_RESULT_BAD_INSTR,
    MIPSU_RESULT_BAD_FN,
    MIPSU_RESULT_BAD_REG,
    MIPSU_RESULT_FIELD_OVERFLOW,
    MIPSU_RESULT_FIELD_SIGN,
    MIPSU_RESULT_SKIPPED,
    MIPSU_RESULT_INSTR_SIZE,

    MIPSU_RESULT_BUFF_OVERFLOW,
    MIPSU_RESULT_READ_FILE,
    MIPSU_RESULT_OPEN_FILE,
};

enum mipsu_flag {
    MIPSU_FLAG_QUIET    = 1 << 0,
    MIPSU_FLAG_VERBOSE  = 1 << 1,
    MIPSU_FLAG_NO_COLOR = 1 << 2,
    MIPSU_FLAG_NREG     = 1 << 3,
    MIPSU_FLAG_DIMM     = 1 << 4,
    MIPSU_FLAG_STRICT   = 1 << 5,
    MIPSU_FLAG_RAW      = 1 << 6,
};

struct mipsu_reg_entry {
    const char* name;
    const char* num;
};

struct mipsu_op_entry {
    const char*    mnem;
    mipsu_op_fmt_t fmt;
    mipsu_type_t   type;
};

struct mipsu_field {
    mipsu_type_t type;
    uint8_t      op;
    union {

        struct {
            uint8_t rs, rt;

            union {
                struct {
                    uint8_t rd, sh, fn;
                };
                struct {
                    int16_t imm;
                };
            };
        };

        uint32_t addr;
    };
};

struct mipsu_flag_entry {
    const char*  name;
    mipsu_flag_t bit;
    char         shrt;
};

struct mipsu_cmd {
    const char* name;
    mipsu_result_t (*from_file)(mipsu_ctx_t c);
    mipsu_result_t (*from_arg)(const char*, mipsu_ctx_t c);
    mipsu_result_t (*from_args)(size_t, const char**, mipsu_ctx_t c);
};

struct mipsu_ctx {
    mipsu_flag_t flags;
    file_t*      o;
    file_t*      f;
};

static const mipsu_reg_entry_t mipsu_reg_lut[] = {
    [0] = {"zero", "0"}, [1] = {"at", "1"},   [2] = {"v0", "2"},
    [3] = {"v1", "3"},   [4] = {"a0", "4"},   [5] = {"a1", "5"},
    [6] = {"a2", "6"},   [7] = {"a3", "7"},   [8] = {"t0", "8"},
    [9] = {"t1", "9"},   [10] = {"t2", "10"}, [11] = {"t3", "11"},
    [12] = {"t4", "12"}, [13] = {"t5", "13"}, [14] = {"t6", "14"},
    [15] = {"t7", "15"}, [16] = {"s0", "16"}, [17] = {"s1", "17"},
    [18] = {"s2", "18"}, [19] = {"s3", "19"}, [20] = {"s4", "20"},
    [21] = {"s5", "21"}, [22] = {"s6", "22"}, [23] = {"s7", "23"},
    [24] = {"t8", "24"}, [25] = {"t9", "25"}, [26] = {"k0", "26"},
    [27] = {"k1", "27"}, [28] = {"gp", "28"}, [29] = {"sp", "29"},
    [30] = {"fp", "30"}, [31] = {"ra", "31"},
};
static size_t mipsu_regc = sizeof(mipsu_reg_lut) / sizeof(mipsu_reg_entry_t);

static size_t mipsu_fnv[] = {
    0x00, 0x02, 0x03, 0x04, 0x06, 0x07, 0x08, 0x09, 0x0C, 0x0D,
    0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1A, 0x1B, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x2A, 0x2B,
};

static size_t mipsu_opv[] = {
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0C, 0x0D,
    0x0F, 0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B,
};

static size_t mipsu_fnc = sizeof(mipsu_fnv) / sizeof(size_t);
static size_t mipsu_opc = sizeof(mipsu_opv) / sizeof(size_t);

static const mipsu_op_entry_t mipsu_fn_lut[64] = {
    /* Shift */
    [0x00] = {"sll", MIPSU_OP_FMT_RD_RT_SH, MIPSU_TYPE_R},

    [0x02] = {"srl", MIPSU_OP_FMT_RD_RT_SH, MIPSU_TYPE_R},
    [0x03] = {"sra", MIPSU_OP_FMT_RD_RT_SH, MIPSU_TYPE_R},
    [0x04] = {"sllv", MIPSU_OP_FMT_RD_RT_RS, MIPSU_TYPE_R},

    [0x06] = {"srlv", MIPSU_OP_FMT_RD_RT_RS, MIPSU_TYPE_R},
    [0x07] = {"srav", MIPSU_OP_FMT_RD_RT_RS, MIPSU_TYPE_R},

    /* Misc */
    [0x08] = {"jalr", MIPSU_OP_FMT_RS, MIPSU_TYPE_R},
    [0x09] = {"jr", MIPSU_OP_FMT_RS, MIPSU_TYPE_R},

    [0x0C] = {"syscall", MIPSU_OP_FMT_NONE, MIPSU_TYPE_R},
    [0x0D] = {"break", MIPSU_OP_FMT_NONE, MIPSU_TYPE_R},

    /* MUL */
    [0x10] = {"mfhi", MIPSU_OP_FMT_RD, MIPSU_TYPE_R},
    [0x11] = {"mthi", MIPSU_OP_FMT_RS, MIPSU_TYPE_R},
    [0x12] = {"mflo", MIPSU_OP_FMT_RD, MIPSU_TYPE_R},
    [0x13] = {"mtlo", MIPSU_OP_FMT_RS, MIPSU_TYPE_R},

    [0x18] = {"mult", MIPSU_OP_FMT_RS_RT, MIPSU_TYPE_R},
    [0x19] = {"multu", MIPSU_OP_FMT_RS_RT, MIPSU_TYPE_R},
    [0x1A] = {"div", MIPSU_OP_FMT_RS_RT, MIPSU_TYPE_R},
    [0x1B] = {"divu", MIPSU_OP_FMT_RS_RT, MIPSU_TYPE_R},

    /* ALU */
    [0x20] = {"add", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x21] = {"addu", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x22] = {"sub", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x23] = {"subu", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x24] = {"and", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x25] = {"or", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x26] = {"xor", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x27] = {"nor", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},

    [0x2A] = {"slt", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
    [0x2B] = {"sltu", MIPSU_OP_FMT_RD_RS_RT, MIPSU_TYPE_R},
};

static const mipsu_op_entry_t mipsu_op_lut[64] = {
    /* Type R */
    [0x00] = {"", MIPSU_OP_FMT_NONE, MIPSU_TYPE_R},

    /* Jump */
    [0x02] = {"j", MIPSU_OP_FMT_ADDR, MIPSU_TYPE_J},
    [0x03] = {"jal", MIPSU_OP_FMT_ADDR, MIPSU_TYPE_J},

    /* Branch */
    [0x04] = {"beq", MIPSU_OP_FMT_RS_RT_IMM, MIPSU_TYPE_I},
    [0x05] = {"bne", MIPSU_OP_FMT_RS_RT_IMM, MIPSU_TYPE_I},
    [0x06] = {"blez", MIPSU_OP_FMT_RS_IMM, MIPSU_TYPE_I},
    [0x07] = {"bgtz", MIPSU_OP_FMT_RS_IMM, MIPSU_TYPE_I},

    /* ALU */
    [0x08] = {"addi", MIPSU_OP_FMT_RT_RS_IMM, MIPSU_TYPE_I},
    [0x09] = {"addiu", MIPSU_OP_FMT_RT_RS_IMM, MIPSU_TYPE_I},
    [0x0C] = {"andi", MIPSU_OP_FMT_RT_RS_IMM, MIPSU_TYPE_I},
    [0x0D] = {"ori", MIPSU_OP_FMT_RT_RS_IMM, MIPSU_TYPE_I},
    [0x0F] = {"lui", MIPSU_OP_FMT_RT_IMM, MIPSU_TYPE_I},

    /* MEM */
    [0x20] = {"lb", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},
    [0x21] = {"lh", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},

    [0x23] = {"lw", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},

    [0x24] = {"lbu", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},
    [0x25] = {"lhu", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},

    [0x28] = {"sb", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},
    [0x29] = {"sh", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},

    [0x2B] = {"sw", MIPSU_OP_FMT_RT_IMM_RS, MIPSU_TYPE_I},
};

static const int8_t mipsu_hex_lut[256] = {
    ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,  ['5'] = 5,
    ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,

    ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
    ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
};

static const char mipsu_type_lut[] = {
    [MIPSU_TYPE_R] = 'R',
    [MIPSU_TYPE_I] = 'I',
    [MIPSU_TYPE_J] = 'J',
};

static const char* mipsu_err_msg_lut[] = {
    [MIPSU_EXIT_OK]       = "ok",
    [MIPSU_EXIT_USAGE]    = "usage error",
    [MIPSU_EXIT_PARSE]    = "parse error",
    [MIPSU_EXIT_INTERNAL] = "internal error",
};

static const char* mipsu_res_msg_lut[] = {
    [MIPSU_RESULT_OK] = "ok",

    [MIPSU_RESULT_BAD_CMD]       = "unknown command",
    [MIPSU_RESULT_BAD_ARGC]      = "invalid number of arguments",
    [MIPSU_RESULT_MISSING_ARGS]  = "missing argument(s)",
    [MIPSU_RESULT_TOO_MANY_ARGS] = "too many arguments",
    [MIPSU_RESULT_FROM_FILE]     = "command does not allow reading from file",
    [MIPSU_RESULT_RAW_STDIN]     = "cannot read raw binary from stdin",
    [MIPSU_RESULT_RAW_STDOUT]    = "cannor write raw binary to stdout",
    [MIPSU_RESULT_STDIN_CHAR]    = "drop '-' to read from stdin",

    [MIPSU_RESULT_BAD_DEC]         = "invalid decimal number",
    [MIPSU_RESULT_BAD_RADIX]       = "unknown radix (base)",
    [MIPSU_RESULT_BAD_HEX]         = "invalid hexadcimal number",
    [MIPSU_RESULT_MISSING_HEXITS]  = "missing hexit(s)",
    [MIPSU_RESULT_TOO_MANY_HEXITS] = "too many hexits",
    [MIPSU_RESULT_BAD_BIN]         = "invalid binary number",
    [MIPSU_RESULT_MISSING_BITS]    = "missing bit(s)",
    [MIPSU_RESULT_TOO_MANY_BITS]   = "too many bits",
    [MIPSU_RESULT_BAD_OP]          = "unknown operation",
    [MIPSU_RESULT_BAD_OP_FMT]      = "invalid operation format",
    [MIPSU_RESULT_BAD_OP_TYPE]     = "unknown operation type",
    [MIPSU_RESULT_BAD_INSTR]       = "unknown instruction",
    [MIPSU_RESULT_BAD_REG]         = "bad register",
    [MIPSU_RESULT_FIELD_OVERFLOW]  = "field overflow",
    [MIPSU_RESULT_FIELD_SIGN]      = "invalid field sign",
    [MIPSU_RESULT_SKIPPED]         = "skipped data",
    [MIPSU_RESULT_INSTR_SIZE]      = "too many instruction arguments",

    [MIPSU_RESULT_BUFF_OVERFLOW] = "buffer overflow",
    [MIPSU_RESULT_READ_FILE]     = "failed to read raw binary file",
    [MIPSU_RESULT_OPEN_FILE]     = "failed to open file",
};

static const mipsu_exit_t mipsu_err_lut[] = {
    [MIPSU_RESULT_OK] = MIPSU_EXIT_OK,

    [MIPSU_RESULT_BAD_CMD]       = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_BAD_ARGC]      = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_MISSING_ARGS]  = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_TOO_MANY_ARGS] = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_FROM_FILE]     = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_RAW_STDIN]     = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_RAW_STDOUT]    = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_STDIN_CHAR]    = MIPSU_EXIT_USAGE,

    [MIPSU_RESULT_BAD_DEC]         = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_RADIX]       = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_HEX]         = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_MISSING_HEXITS]  = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_TOO_MANY_HEXITS] = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_BIN]         = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_MISSING_BITS]    = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_TOO_MANY_BITS]   = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_OP]          = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_OP_FMT]      = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_OP_TYPE]     = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_INSTR]       = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_REG]         = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_FIELD_OVERFLOW]  = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_FIELD_SIGN]      = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_SKIPPED]         = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_INSTR_SIZE]      = MIPSU_EXIT_PARSE,

    [MIPSU_RESULT_BUFF_OVERFLOW] = MIPSU_EXIT_INTERNAL,
    [MIPSU_RESULT_READ_FILE]     = MIPSU_EXIT_INTERNAL,
    [MIPSU_RESULT_OPEN_FILE]     = MIPSU_EXIT_INTERNAL,
};

static const mipsu_flag_entry_t mipsu_flag_lut[] = {
    {"quiet", MIPSU_FLAG_QUIET, 'q'},
    {"verbose", MIPSU_FLAG_VERBOSE, 'v'},
    {"no-color", MIPSU_FLAG_NO_COLOR, 0},
    {"nreg", MIPSU_FLAG_NREG, 'n'},
    {"dimm", MIPSU_FLAG_DIMM, 'd'},
    {"strict", MIPSU_FLAG_STRICT, 's'},
    {"raw", MIPSU_FLAG_RAW, 0},
};

static const size_t mipsu_flagc =
    sizeof(mipsu_flag_lut) / sizeof(mipsu_flag_entry_t);

static const char* mipsu_usage =

    "MIPS32 utilities\n"
    "\n"
    "usage:\n"
    "  mipsu decode <hex | bin>\n"
    "  mipsu disasm <hex | bin>\n"
    "  mipsu disasm -f <file>\n"
    "  mipsu encode -R <rs> <rt> <rd> <sh> <fn>\n"
    "  mipsu encode -I <op> <rs> <rt> <imm>\n"
    "  mipsu encode -J <op> <addr>\n"
    "  mipsu asm    <mips>\n"
    "  mipsu asm    -f <file>\n"
    "  mipsu --version\n"
    "  mipsu --help | -h\n"
    "\n"
    "commands:\n"
    "  decode  32bit instruction -> bitfield\n"
    "  disasm  32bit instruction -> assembly\n"
    "  encode  bitfield -> 32bit instruction\n"
    "  asm     assembly -> 32bit instruction\n"
    "\n"
    "flags:\n"
    "  -q, --quiet     minimal output\n"
    "  -v, --verbose   maximal output\n"
    "      --no-color  plain output\n"
    "  -n, --nreg      use numbers when formatting registers\n"
    "  -d, --dimm      use base 10 when formatting immediates\n"
    "  -s, --strict    enable strict parsing\n"
    "      --raw       handle raw binary files\n"
    "\n"
    "options:\n"
    "  -o <file>, --output <file>  Specify an output file\n"
    "  -f <file>, --file   <file>  Specify an input file\n";

static const size_t mipsu_word_size = sizeof(mipsu_word_t);

static const uint8_t mipsu_op_offset = 26;
static const uint8_t mipsu_rs_offset = 21;
static const uint8_t mipsu_rt_offset = 16;
static const uint8_t mipsu_rd_offset = 11;
static const uint8_t mipsu_sh_offset = 6;

static const uint8_t  mipsu_6bit_mask  = 0b00111111;
static const uint8_t  mipsu_5bit_mask  = 0b00011111;
static const uint16_t mipsu_16bit_mask = 0xFFFF;
static const uint32_t mipsu_26bit_mask = 0x03FFFFFF;

static const char* mipsu_fmt_none     = "%-8s\n";
static const char* mipsu_fmt_hex8     = "%-8s 0x%08X\n";
static const char* mipsu_fmt_1r       = "%-8s $%-4s\n";
static const char* mipsu_fmt_2r       = "%-8s $%-4s, $%-4s\n";
static const char* mipsu_fmt_3r       = "%-8s $%-4s, $%-4s, $%-4s\n";
static const char* mipsu_fmt_2r_hex2  = "%-8s $%-4s, $%-4s, 0x%02hhX\n";
static const char* mipsu_fmt_1r_dec6  = "%-8s $%-4s, %6hi\n";
static const char* mipsu_fmt_2r_dec6  = "%-8s $%-4s, $%-4s, %6hi\n";
static const char* mipsu_fmt_1r_hex4  = "%-8s $%-4s, 0x%04hX\n";
static const char* mipsu_fmt_2r_hex4  = "%-8s $%-4s, $%-4s, 0x%04hX\n";
static const char* mipsu_fmt_r_hex4_r = "%-8s $%-4s, 0x%04hX( $%s )\n";

static const char mipsu_color_err = '1';
static const char mipsu_color_wrn = '3';

static char mipsu_chr_buff[1024];
static char mipsu_cp_buff[1024];

static const size_t mipsu_chr_len = sizeof(mipsu_chr_buff);
static const size_t mipsu_cp_len  = sizeof(mipsu_cp_buff);
static const size_t mipsu_cp_last = mipsu_cp_len - 1;

/* === CLI utils === */

static bool_t mipsu_get_flag(mipsu_ctx_t c, mipsu_flag_t f) {
    return c.flags & f;
}
static void mipsu_set_flag(mipsu_ctx_t* c, mipsu_flag_t f) { c->flags |= f; }

static void mipsu_cerr(const char* msg, char col, const char* v,
                       mipsu_ctx_t c) {
    char       start[6];
    const char reset[] = "\033[0m";

    bool_t no_col = mipsu_get_flag(c, MIPSU_FLAG_NO_COLOR);
    sprintf(start, "\033[3%cm", col);

    if (v)
        fprintf(stderr,
                "%smipsu%s: %s. '%s'\n",
                no_col ? "" : start,
                no_col ? "" : reset,
                msg,
                v);
    else
        fprintf(stderr,
                "%smipsu%s: %s.\n",
                no_col ? "" : start,
                no_col ? "" : reset,
                msg);
}

static void mipsu_err(const char* msg, mipsu_ctx_t c) {
    mipsu_cerr(msg, mipsu_color_err, NULL, c);
}

static void mipsu_errv(const char* msg, const char* v, mipsu_ctx_t c) {
    mipsu_cerr(msg, mipsu_color_err, v, c);
}

static void mipsu_wrnr(mipsu_result_t r, mipsu_ctx_t c) {
    mipsu_cerr(mipsu_res_msg_lut[r], mipsu_color_wrn, NULL, c);
}

static void mipsu_wrnrv(mipsu_result_t r, const char* v, mipsu_ctx_t c) {
    mipsu_cerr(mipsu_res_msg_lut[r], mipsu_color_wrn, v, c);
}

static void mipsu_exit_ok() { exit(MIPSU_EXIT_OK); }

static void mipsu_exit(mipsu_exit_t e, mipsu_ctx_t c) {
    if (e) mipsu_err(mipsu_err_msg_lut[e], c);
    exit(e);
}

static void mipsu_exitr(mipsu_result_t r, mipsu_ctx_t c) {
    if (r) mipsu_err(mipsu_res_msg_lut[r], c);
    mipsu_exit(mipsu_err_lut[r], c);
}

static void mipsu_exitrv(mipsu_result_t r, const char* v, mipsu_ctx_t c) {
    if (r) mipsu_errv(mipsu_res_msg_lut[r], v, c);
    mipsu_exit(mipsu_err_lut[r], c);
}

/* === Parsing === */

static bool_t mipsu_ignore(char c) {
    switch (c) {
    case ' ':
    case '\n':
    case ',':
    case '(':
    case ')':
        return true;
    default:
        return false;
    }
}

static bool_t mipsu_bin_spec(char c) { return c == 'B' || c == 'b'; }
static bool_t mipsu_hex_spec(char c) { return c == 'X' || c == 'x'; }

static mipsu_result_t mipsu_bin_valid(const char* s, size_t n) {
    size_t i;

    for (i = 0; i < n; ++i) {
        if (!s[i]) return MIPSU_RESULT_MISSING_BITS;
        if (s[i] != '0' && s[i] != '1') return MIPSU_RESULT_BAD_BIN;
    }

    return s[n] ? MIPSU_RESULT_TOO_MANY_BITS : MIPSU_RESULT_OK;
}
static mipsu_result_t mipsu_hex_valid(const char* s, size_t n) {
    size_t i;

    for (i = 0; i < n; ++i) {
        if (!s[i]) return MIPSU_RESULT_MISSING_HEXITS;

        if (s[i] != '0' && !mipsu_hex_lut[(size_t)s[i]])
            return MIPSU_RESULT_BAD_HEX;
    }

    return s[n] ? MIPSU_RESULT_TOO_MANY_HEXITS : MIPSU_RESULT_OK;
}

static mipsu_word_t mipsu_get_value(const char* s, size_t n, uint8_t sh) {
    size_t       i;
    mipsu_word_t v = 0;

    for (i = 0; i < n; ++i)
        v = (v << sh) | (mipsu_word_t)mipsu_hex_lut[(size_t)s[i]];

    return v;
}

static mipsu_result_t mipsu_parse_decimal(const char* s, int64_t* out, bool_t u,
                                          uint8_t b) {
    char* e;

    *out = strtol(s, &e, 10);

    if (*e) return MIPSU_RESULT_BAD_DEC;

    if (u) {
        if (*out < 0) return MIPSU_RESULT_FIELD_SIGN;
        if (*out >= (2ll << b)) return MIPSU_RESULT_FIELD_OVERFLOW;

    } else if (labs(*out) >= (1ll << b))
        return MIPSU_RESULT_FIELD_OVERFLOW;

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_parse_value(const char* s, mipsu_word_t* out,
                                        bool_t u, uint8_t b) {
    size_t         n  = b;
    uint8_t        sh = 1;
    mipsu_result_t r;
    int64_t        l;

    if (s[0] != '0') {
        r = mipsu_parse_decimal(s, &l, u, b);
        if (!r) *out = l;
        return r;
    }

    if (mipsu_hex_spec(s[1])) {
        n  = (b + 3) / 4;
        sh = 4;
        r  = mipsu_hex_valid(s + 2, n);
        if (r) return r;
    } else if (mipsu_bin_spec(s[1])) {
        r = mipsu_bin_valid(s + 2, n);
        if (r) return r;
    } else
        return MIPSU_RESULT_BAD_RADIX;

    *out = mipsu_get_value(s + 2, n, sh);

    if (b != 32 && *out >= (1u << b)) return MIPSU_RESULT_FIELD_OVERFLOW;

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_parse_word(const char* s, mipsu_word_t* out) {
    return mipsu_parse_value(s, out, true, mipsu_word_size * 8);
}

static mipsu_result_t mipsu_parse_imm(const char* s, int16_t* i) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_value(s, &w, false, 16);
    if (!r) *i = w;

    return r;
}

static mipsu_result_t mipsu_parse_sh(const char* s, uint8_t* sh) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_value(s, &w, true, 5);
    if (!r) *sh = w;

    return r;
}

static mipsu_result_t mipsu_parse_addr(const char* s, uint32_t* addr) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_word(s, &w);
    if (!r) *addr = w;

    if (w >= (1u << 26)) return MIPSU_RESULT_FIELD_OVERFLOW;

    return r;
}

static mipsu_result_t mipsu_parse_type(const char* s, mipsu_type_t* out,
                                       mipsu_ctx_t c) {
    if (strlen(s) > 2) return MIPSU_RESULT_BAD_OP_TYPE;

    bool_t prefixed = s[0] == '-';
    bool_t strict   = mipsu_get_flag(c, MIPSU_FLAG_STRICT);

    if (!prefixed && strict) return MIPSU_RESULT_BAD_OP_TYPE;

    if (prefixed) ++s;

    switch (s[0]) {
    case 'R':
    case 'r':
        *out = MIPSU_TYPE_R;
        break;
    case 'I':
    case 'i':
        *out = MIPSU_TYPE_I;
        break;
    case 'J':
    case 'j':
        *out = MIPSU_TYPE_J;
        break;
    default:
        return MIPSU_RESULT_BAD_OP_TYPE;
    }

    return s[1] ? MIPSU_RESULT_BAD_OP_TYPE : MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_parse_field_hlpr(const char* s, const char* msg,
                                             uint8_t* f, uint8_t n,
                                             mipsu_ctx_t c) {
    mipsu_result_t r;
    mipsu_word_t   w;

    r  = mipsu_parse_value(s, &w, true, n);
    *f = w;

    if (r) mipsu_errv(msg, s, c);

    return r;
}

static mipsu_result_t mipsu_parse_r_field(int32_t argc, const char** argv,
                                          mipsu_field_t* out, mipsu_ctx_t c) {
    mipsu_result_t r;

    if (argc < 6) return MIPSU_RESULT_MISSING_ARGS;
    if (argc > 6) return MIPSU_RESULT_TOO_MANY_ARGS;

    out->type = MIPSU_TYPE_R;
    out->op   = 0;

    r = mipsu_parse_field_hlpr(argv[1], "in <rs>", &out->rs, 5, c);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[2], "in <rt>", &out->rt, 5, c);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[3], "in <rd>", &out->rd, 5, c);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[4], "in <sh>", &out->sh, 5, c);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[5], "in <fn>", &out->fn, 6, c);

    return r;
}

static mipsu_result_t mipsu_parse_i_field(int32_t argc, const char** argv,
                                          mipsu_field_t* out, mipsu_ctx_t c) {
    mipsu_result_t r;

    if (argc < 5) return MIPSU_RESULT_MISSING_ARGS;
    if (argc > 5) return MIPSU_RESULT_TOO_MANY_ARGS;

    out->type = MIPSU_TYPE_I;

    r = mipsu_parse_field_hlpr(argv[1], "in <op>", &out->op, 6, c);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[2], "in <rs>", &out->rs, 5, c);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[3], "in <rt>", &out->rt, 5, c);
    if (r) return r;

    r = mipsu_parse_imm(argv[4], &out->imm);
    if (r) mipsu_errv("in <imm>", argv[4], c);

    return r;
}

static mipsu_result_t mipsu_parse_j_field(int32_t argc, const char** argv,
                                          mipsu_field_t* out, mipsu_ctx_t c) {
    mipsu_result_t r;
    mipsu_word_t   w;

    if (argc < 3) return MIPSU_RESULT_MISSING_ARGS;
    if (argc > 3) return MIPSU_RESULT_TOO_MANY_ARGS;

    out->type = MIPSU_TYPE_I;

    r = mipsu_parse_field_hlpr(argv[1], "in <op>", &out->op, 6, c);
    if (r) return r;

    r       = mipsu_parse_value(argv[2], &w, false, 26);
    out->rs = w;
    if (r) mipsu_errv("in <addr>", argv[2], c);

    return r;
}

static mipsu_result_t mipsu_parse_field(int32_t argc, const char** argv,
                                        mipsu_field_t* out, mipsu_ctx_t c) {
    mipsu_type_t   t;
    mipsu_result_t r;

    if (argc < 1) return MIPSU_RESULT_MISSING_ARGS;

    r = mipsu_parse_type(argv[0], &t, c);

    if (r) return r;

    switch (t) {
    case MIPSU_TYPE_R:
        return mipsu_parse_r_field(argc, argv, out, c);
    case MIPSU_TYPE_I:
        return mipsu_parse_i_field(argc, argv, out, c);
    case MIPSU_TYPE_J:
        return mipsu_parse_j_field(argc, argv, out, c);
    }
}

static mipsu_result_t mipsu_parse_op(const char* s, mipsu_op_entry_t* e,
                                     uint8_t* o) {
    size_t i;

    for (i = 0; i < mipsu_fnc; ++i) {
        *o = mipsu_fnv[i];
        *e = mipsu_fn_lut[*o];
        if (!strcmp(s, e->mnem)) return MIPSU_RESULT_OK;
    }

    for (i = 0; i < mipsu_opc; ++i) {
        *o = mipsu_opv[i];
        *e = mipsu_op_lut[*o];
        if (!strcmp(s, e->mnem)) return MIPSU_RESULT_OK;
    }

    return MIPSU_RESULT_BAD_OP;
}

static mipsu_result_t mipsu_parse_reg(const char* s, uint8_t* r,
                                      mipsu_ctx_t c) {

    bool_t prefixed = s[0] == '$';
    bool_t strict   = mipsu_get_flag(c, MIPSU_FLAG_STRICT);

    if (!prefixed && strict) return MIPSU_RESULT_BAD_REG;

    if (prefixed) ++s;

    for (*r = 0; *r < mipsu_regc; ++(*r))
        if (!strcmp(s, mipsu_reg_lut[*r].name)) return MIPSU_RESULT_OK;
    for (*r = 0; *r < mipsu_regc; ++(*r))
        if (!strcmp(s, mipsu_reg_lut[*r].num)) return MIPSU_RESULT_OK;

    return MIPSU_RESULT_BAD_REG;
}

static mipsu_result_t mipsu_parse_1r(size_t n, const char** a, uint8_t* r1,
                                     mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 2) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    return r;
}

static mipsu_result_t mipsu_parse_2r(size_t n, const char** a, uint8_t* r1,
                                     uint8_t* r2, mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 3) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    if (r) return r;

    r = mipsu_parse_reg(a[2], r2, c);
    return r;
}

static mipsu_result_t mipsu_parse_3r(size_t n, const char** a, uint8_t* r1,
                                     uint8_t* r2, uint8_t* r3, mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 4) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    if (r) return r;

    r = mipsu_parse_reg(a[2], r2, c);
    if (r) return r;

    r = mipsu_parse_reg(a[3], r3, c);
    return r;
}

static mipsu_result_t mipsu_parse_2r_sh(size_t n, const char** a, uint8_t* r1,
                                        uint8_t* r2, uint8_t* sh,
                                        mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 4) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    if (r) return r;

    r = mipsu_parse_reg(a[2], r2, c);
    if (r) return r;

    r = mipsu_parse_sh(a[3], sh);
    return r;
}

static mipsu_result_t mipsu_parse_1r_imm(size_t n, const char** a, uint8_t* r1,
                                         int16_t* imm, mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 3) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    if (r) return r;

    r = mipsu_parse_imm(a[2], imm);
    return r;
}

static mipsu_result_t mipsu_parse_2r_imm(size_t n, const char** a, uint8_t* r1,
                                         uint8_t* r2, int16_t* imm,
                                         mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 4) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    if (r) return r;

    r = mipsu_parse_reg(a[2], r2, c);
    if (r) return r;

    r = mipsu_parse_imm(a[3], imm);
    return r;
}

static mipsu_result_t mipsu_parse_r_imm_r(size_t n, const char** a, uint8_t* r1,
                                          int16_t* imm, uint8_t* r2,
                                          mipsu_ctx_t c) {
    mipsu_result_t r;

    if (n != 4) return MIPSU_RESULT_BAD_OP_FMT;

    r = mipsu_parse_reg(a[1], r1, c);
    if (r) return r;

    r = mipsu_parse_imm(a[2], imm);
    if (r) return r;

    r = mipsu_parse_reg(a[3], r2, c);
    return r;
}

static mipsu_result_t mipsu_parse_asm(const char* s, const char** a,
                                      size_t* n) {
    size_t i;

    *n = 0;

    strncpy(mipsu_cp_buff, s, mipsu_cp_last);

    if (mipsu_cp_buff[mipsu_cp_last]) return MIPSU_RESULT_BUFF_OVERFLOW;

    for (i = 0; mipsu_cp_buff[i]; ++i) {

        if (mipsu_ignore(mipsu_cp_buff[i])) continue;

        if (*n < 4)
            a[(*n)++] = mipsu_cp_buff + i;

        else
            return MIPSU_RESULT_INSTR_SIZE;

        while (!mipsu_ignore(mipsu_cp_buff[i]) && mipsu_cp_buff[i])
            ++i;

        mipsu_cp_buff[i] = 0;
    }

    return MIPSU_RESULT_OK;
}

/* === Formatting === */

static size_t mipsu_fmt_field(mipsu_word_t w, mipsu_field_t f, char* b,
                              mipsu_ctx_t c) {

    const mipsu_reg_entry_t rse = mipsu_reg_lut[f.rs];
    const mipsu_reg_entry_t rte = mipsu_reg_lut[f.rt];
    const mipsu_reg_entry_t rde = mipsu_reg_lut[f.rd];

    bool_t nreg = mipsu_get_flag(c, MIPSU_FLAG_NREG);

    const char* rs = nreg ? rse.num : rse.name;
    const char* rt = nreg ? rte.num : rte.name;
    const char* rd = nreg ? rde.num : rde.name;

    const char* fn = mipsu_fn_lut[f.fn].mnem;
    const char* op = mipsu_op_lut[f.op].mnem;

    size_t o = 0;

    char t;

    if (!op || (f.type == MIPSU_TYPE_R && !fn))
        mipsu_wrnr(MIPSU_RESULT_BAD_INSTR, c);

    if (!mipsu_get_flag(c, MIPSU_FLAG_QUIET)) {
        t = mipsu_type_lut[f.type];

        o += sprintf(b + o, "hex:   0x%08X\n", w);
        o += sprintf(b + o, "type:  %c%s\n", t, op ? "" : "?");
        o += sprintf(b + o, "--------\n");
    }

    switch (f.type) {
    case MIPSU_TYPE_R:
        o += sprintf(b + o, "rs:  0x%02hhX  ($%s)\n", f.rs, rs);
        o += sprintf(b + o, "rt:  0x%02hhX  ($%s)\n", f.rt, rt);
        o += sprintf(b + o, "rd:  0x%02hhX  ($%s)\n", f.rd, rd);
        o += sprintf(b + o, "sh:  0x%02hhX  (%hhu)\n", f.sh, f.sh);
        o += sprintf(b + o, "fn:  0x%02hhX  (%s)\n", f.fn, fn ? fn : "?");
        break;
    case MIPSU_TYPE_I:
        o += sprintf(b + o, "op:   0x%02hhX    (%s)\n", f.op, op ? op : "?");
        o += sprintf(b + o, "rs:   0x%02hhX    ($%s)\n", f.rs, rs);
        o += sprintf(b + o, "rt:   0x%02hhX    ($%s)\n", f.rt, rt);
        o += sprintf(b + o, "imm:  0x%04hX  (%hi)\n", f.imm, f.imm);
        break;
    case MIPSU_TYPE_J:
        o += sprintf(b + o, "op:    0x%02hhX      (%s)\n", f.op, op ? op : "?");
        o += sprintf(b + o, "addr:  0x%08X  (%u)\n", f.addr, f.addr);
        break;
    }

    return o;
}

/* === Decoding === */

static uint8_t mipsu_op(mipsu_word_t w) {
    return (w >> mipsu_op_offset) & mipsu_6bit_mask;
}
static uint8_t mipsu_rs(mipsu_word_t w) {
    return (w >> mipsu_rs_offset) & mipsu_5bit_mask;
}
static uint8_t mipsu_rt(mipsu_word_t w) {
    return (w >> mipsu_rt_offset) & mipsu_5bit_mask;
}
static uint8_t mipsu_rd(mipsu_word_t w) {
    return (w >> mipsu_rd_offset) & mipsu_5bit_mask;
}
static uint8_t mipsu_sh(mipsu_word_t w) {
    return (w >> mipsu_sh_offset) & mipsu_5bit_mask;
}
static uint8_t  mipsu_fn(mipsu_word_t w) { return w & mipsu_6bit_mask; }
static int16_t  mipsu_imm(mipsu_word_t w) { return w & mipsu_16bit_mask; }
static uint32_t mipsu_addr(mipsu_word_t w) { return w & mipsu_26bit_mask; }

static mipsu_field_t mipsu_decode(mipsu_word_t w) {
    mipsu_field_t f;
    f.op = mipsu_op(w);

    if (f.op == 0) {
        f.type = MIPSU_TYPE_R;
        f.rs   = mipsu_rs(w);
        f.rt   = mipsu_rt(w);
        f.rd   = mipsu_rd(w);
        f.sh   = mipsu_sh(w);
        f.fn   = mipsu_fn(w);
    } else if (f.op == 2 || f.op == 3) {
        f.type = MIPSU_TYPE_J;
        f.addr = mipsu_addr(w);
    } else {
        f.type = MIPSU_TYPE_I;
        f.rs   = mipsu_rs(w);
        f.rt   = mipsu_rt(w);
        f.imm  = mipsu_imm(w);
    }

    return f;
}

/* === Encoding === */

static mipsu_word_t mipsu_encode(mipsu_field_t f) {

    switch (f.type) {
    case MIPSU_TYPE_R:
        return (((mipsu_word_t)f.op << mipsu_op_offset) |
                ((mipsu_word_t)f.rs << mipsu_rs_offset) |
                ((mipsu_word_t)f.rt << mipsu_rt_offset) |
                ((mipsu_word_t)f.rd << mipsu_rd_offset) |
                ((mipsu_word_t)f.sh << mipsu_sh_offset | (mipsu_word_t)f.fn));
    case MIPSU_TYPE_I:
        return (((mipsu_word_t)f.op << mipsu_op_offset) |
                ((mipsu_word_t)f.rs << mipsu_rs_offset) |
                ((mipsu_word_t)f.rt << mipsu_rt_offset) |
                ((mipsu_word_t)f.imm & mipsu_16bit_mask));
    case MIPSU_TYPE_J:
        return (((mipsu_word_t)f.op << mipsu_op_offset) | (mipsu_word_t)f.addr);
    }
}

/* === Disassembly === */

static size_t mipsu_disasm(mipsu_field_t f, char* b, mipsu_ctx_t c) {

    const mipsu_reg_entry_t rse = mipsu_reg_lut[f.rs];
    const mipsu_reg_entry_t rte = mipsu_reg_lut[f.rt];
    const mipsu_reg_entry_t rde = mipsu_reg_lut[f.rd];

    bool_t nreg = mipsu_get_flag(c, MIPSU_FLAG_NREG);
    bool_t dimm = mipsu_get_flag(c, MIPSU_FLAG_DIMM);

    const char* fmt_1r_imm = dimm ? mipsu_fmt_1r_dec6 : mipsu_fmt_1r_hex4;
    const char* fmt_2r_imm = dimm ? mipsu_fmt_2r_dec6 : mipsu_fmt_2r_hex4;

    const char* rs = nreg ? rse.num : rse.name;
    const char* rt = nreg ? rte.num : rte.name;
    const char* rd = nreg ? rde.num : rde.name;

    mipsu_op_entry_t e;

    if (f.type == MIPSU_TYPE_R)
        e = mipsu_fn_lut[f.fn];
    else
        e = mipsu_op_lut[f.op];

    switch (e.fmt) {
    case MIPSU_OP_FMT_UNKNOWN:
        return sprintf(b, mipsu_fmt_hex8, ".word", mipsu_encode(f));
    case MIPSU_OP_FMT_NONE:
        return sprintf(b, mipsu_fmt_none, e.mnem);

    case MIPSU_OP_FMT_RS:
        return sprintf(b, mipsu_fmt_1r, e.mnem, rs);
    case MIPSU_OP_FMT_RD:
        return sprintf(b, mipsu_fmt_1r, e.mnem, rd);
    case MIPSU_OP_FMT_RS_RT:
        return sprintf(b, mipsu_fmt_2r, e.mnem, rs, rt);
    case MIPSU_OP_FMT_RD_RT_RS:
        return sprintf(b, mipsu_fmt_3r, e.mnem, rd, rt, rs);
    case MIPSU_OP_FMT_RD_RT_SH:
        return sprintf(b, mipsu_fmt_2r_hex2, e.mnem, rd, rt, f.sh);
    case MIPSU_OP_FMT_RD_RS_RT:
        return sprintf(b, mipsu_fmt_3r, e.mnem, rd, rs, rt);

    case MIPSU_OP_FMT_RS_IMM:
        return sprintf(b, fmt_1r_imm, e.mnem, rs, f.imm);
    case MIPSU_OP_FMT_RT_IMM:
        return sprintf(b, fmt_1r_imm, e.mnem, rt, f.imm);
    case MIPSU_OP_FMT_RT_IMM_RS:
        return sprintf(b, mipsu_fmt_r_hex4_r, e.mnem, rt, f.imm, rs);
    case MIPSU_OP_FMT_RT_RS_IMM:
        return sprintf(b, fmt_2r_imm, e.mnem, rt, rs, f.imm);
    case MIPSU_OP_FMT_RS_RT_IMM:
        return sprintf(b, fmt_2r_imm, e.mnem, rs, rt, f.imm);

    case MIPSU_OP_FMT_ADDR:
        return sprintf(b, mipsu_fmt_hex8, e.mnem, f.addr);
    }
}

/* === Assembly === */

static mipsu_result_t mipsu_asm(size_t n, const char** a, mipsu_field_t* f,
                                mipsu_ctx_t c) {
    mipsu_op_entry_t e;
    mipsu_result_t   r;

    memset(f, 0, sizeof(mipsu_field_t));

    r = mipsu_parse_op(a[0], &e, &f->op);
    if (r) return r;

    f->type = e.type;

    if (f->type == MIPSU_TYPE_R) {
        f->fn = f->op;
        f->op = 0;
    }

    switch (e.fmt) {
    case MIPSU_OP_FMT_UNKNOWN:
        return MIPSU_RESULT_BAD_OP;
    case MIPSU_OP_FMT_NONE:
        return MIPSU_RESULT_OK;

    case MIPSU_OP_FMT_RS:
        return mipsu_parse_1r(n, a, &f->rs, c);
    case MIPSU_OP_FMT_RD:
        return mipsu_parse_1r(n, a, &f->rd, c);
    case MIPSU_OP_FMT_RS_RT:
        return mipsu_parse_2r(n, a, &f->rs, &f->rt, c);
    case MIPSU_OP_FMT_RD_RS_RT:
        return mipsu_parse_3r(n, a, &f->rd, &f->rs, &f->rt, c);
    case MIPSU_OP_FMT_RD_RT_RS:
        return mipsu_parse_3r(n, a, &f->rd, &f->rt, &f->rs, c);
    case MIPSU_OP_FMT_RD_RT_SH:
        return mipsu_parse_2r_sh(n, a, &f->rd, &f->rt, &f->sh, c);

    case MIPSU_OP_FMT_RS_IMM:
        return mipsu_parse_1r_imm(n, a, &f->rs, &f->imm, c);
    case MIPSU_OP_FMT_RT_IMM:
        return mipsu_parse_1r_imm(n, a, &f->rt, &f->imm, c);
    case MIPSU_OP_FMT_RT_IMM_RS:
        return mipsu_parse_r_imm_r(n, a, &f->rt, &f->imm, &f->rs, c);
    case MIPSU_OP_FMT_RT_RS_IMM:
        return mipsu_parse_2r_imm(n, a, &f->rt, &f->rs, &f->imm, c);
    case MIPSU_OP_FMT_RS_RT_IMM:
        return mipsu_parse_2r_imm(n, a, &f->rs, &f->rt, &f->imm, c);

    case MIPSU_OP_FMT_ADDR:
        if (n != 2) return MIPSU_RESULT_BAD_OP_FMT;
        return mipsu_parse_addr(a[1], &f->addr);
    }
}

/* === Dumping === */

static void mipsu_dump_field(mipsu_word_t w, mipsu_field_t f, mipsu_ctx_t c) {
    mipsu_fmt_field(w, f, mipsu_chr_buff, c);
    fputs(mipsu_chr_buff, c.o);
}

static void mipsu_dump_word(mipsu_word_t w, mipsu_ctx_t c) {
    if (mipsu_get_flag(c, MIPSU_FLAG_RAW))
        fwrite(&w, mipsu_word_size, 1, c.o);
    else
        fprintf(c.o, "0x%08X\n", w);
}

static void mipsu_dump_mnem(mipsu_field_t f, mipsu_ctx_t c) {
    mipsu_disasm(f, mipsu_chr_buff, c);
    fputs(mipsu_chr_buff, c.o);
}

static void mipsu_dump_instr(mipsu_word_t w, mipsu_field_t f, mipsu_ctx_t c) {
    mipsu_disasm(f, mipsu_chr_buff, c);
    fprintf(c.o, "0x%08X  %s", w, mipsu_chr_buff);
}

static void mipsu_dump_decoded(mipsu_word_t w, mipsu_ctx_t c) {
    mipsu_field_t f = mipsu_decode(w);
    mipsu_dump_field(w, f, c);
}

static void mipsu_dump_encoded(mipsu_field_t f, mipsu_ctx_t c) {
    mipsu_word_t w = mipsu_encode(f);

    if (mipsu_get_flag(c, MIPSU_FLAG_QUIET))
        mipsu_dump_word(w, c);
    else
        mipsu_dump_field(w, f, c);
}

static void mipsu_dump_disasm(mipsu_word_t w, mipsu_ctx_t c) {
    mipsu_field_t f = mipsu_decode(w);

    if (mipsu_get_flag(c, MIPSU_FLAG_QUIET))
        mipsu_dump_mnem(f, c);
    else
        mipsu_dump_instr(w, f, c);
}

static void mipsu_dump_asm(mipsu_field_t f, mipsu_ctx_t c) {
    mipsu_word_t w = mipsu_encode(f);

    if (mipsu_get_flag(c, MIPSU_FLAG_QUIET))
        mipsu_dump_word(w, c);
    else
        mipsu_dump_instr(w, f, c);
}

/* === Command Line Interface === */

static mipsu_result_t mipsu_arg_decode(const char* s, mipsu_ctx_t c) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_word(s, &w);

    if (r) return r;

    mipsu_dump_decoded(w, c);

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_args_encode(size_t n, const char** args,
                                        mipsu_ctx_t c) {

    mipsu_field_t  f;
    mipsu_result_t r;

    r = mipsu_parse_field(n, args, &f, c);

    if (!r) mipsu_dump_encoded(f, c);

    return r;
}

static mipsu_result_t mipsu_raw_disasm(mipsu_ctx_t c) {
    mipsu_word_t w;
    size_t       s, i;

    if (c.f == stdin) return MIPSU_RESULT_RAW_STDIN;

    fseek(c.f, 0, SEEK_END);
    s = ftell(c.f);
    fseek(c.f, 0, SEEK_SET);

    if (s > mipsu_cp_len) return MIPSU_RESULT_BUFF_OVERFLOW;

    i = fread(mipsu_cp_buff, 1, s, c.f);

    if (i != s) return MIPSU_RESULT_READ_FILE;

    for (i = 0; i < s; i += mipsu_word_size) {
        w = *(mipsu_word_t*)(mipsu_cp_buff + i);

        mipsu_dump_disasm(w, c);
    }

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_file_disasm(mipsu_ctx_t c) {
    mipsu_word_t   w;
    bool_t         s = false;
    mipsu_result_t r;

    bool_t raw = mipsu_get_flag(c, MIPSU_FLAG_RAW);

    if (raw) return mipsu_raw_disasm(c);

    while (fgets(mipsu_chr_buff, mipsu_chr_len, c.f)) {

        *strchr(mipsu_chr_buff, '\n') = 0;

        r = mipsu_parse_word(mipsu_chr_buff, &w);

        if (r) {
            if (mipsu_get_flag(c, MIPSU_FLAG_STRICT)) return r;
            mipsu_wrnrv(r, mipsu_chr_buff, c);
            s = true;
            continue;
        }

        mipsu_dump_disasm(w, c);
    }

    return s ? MIPSU_RESULT_SKIPPED : MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_arg_disasm(const char* s, mipsu_ctx_t c) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_word(s, &w);

    if (r) return r;

    mipsu_dump_disasm(w, c);

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_args_asm(size_t n, const char** args,
                                     mipsu_ctx_t c) {
    mipsu_result_t r;
    mipsu_field_t  f = {};

    r = mipsu_asm(n, args, &f, c);
    if (r) return r;

    mipsu_dump_asm(f, c);

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_arg_asm(const char* s, mipsu_ctx_t c) {
    const char* a[4] = {};
    size_t      n;

    mipsu_result_t r = mipsu_parse_asm(s, a, &n);

    if (r) return r;

    return mipsu_args_asm(n, a, c);
}

static mipsu_result_t mipsu_file_asm(mipsu_ctx_t c) {
    const char*    a[4] = {};
    size_t         n;
    bool_t         s = false;
    mipsu_result_t r;
    mipsu_field_t  f;

    if (c.o == stdout && mipsu_get_flag(c, MIPSU_FLAG_RAW))
        return MIPSU_RESULT_RAW_STDOUT;

    while (fgets(mipsu_chr_buff, mipsu_chr_len, c.f)) {

        *strchr(mipsu_chr_buff, '\n') = 0;

        if (!strlen(mipsu_chr_buff)) continue;

        r = mipsu_parse_asm(mipsu_chr_buff, a, &n);

        if (!r) r = mipsu_asm(n, a, &f, c);

        if (r) {
            if (mipsu_get_flag(c, MIPSU_FLAG_STRICT)) return r;
            mipsu_wrnrv(r, mipsu_chr_buff, c);
            s = true;
            continue;
        }

        mipsu_dump_asm(f, c);
    }

    return s ? MIPSU_RESULT_SKIPPED : MIPSU_RESULT_OK;
}

static void mipsu_version() {
    printf("mipsu %s\n", MIPSU_VERSION);
    mipsu_exit_ok();
}

static void mipsu_help() {
    puts(mipsu_usage);
    mipsu_exit_ok();
}

static const mipsu_cmd_t mipsu_cmdv[] = {
    {
        "decode",
        NULL,
        mipsu_arg_decode,
        NULL,
    },
    {
        "disasm",
        mipsu_file_disasm,
        mipsu_arg_disasm,
        NULL,
    },
    {
        "encode",
        NULL,
        NULL,
        mipsu_args_encode,
    },
    {
        "asm",
        mipsu_file_asm,
        mipsu_arg_asm,
        mipsu_args_asm,
    },
};

static const size_t mipsu_cmdc = sizeof(mipsu_cmdv) / sizeof(mipsu_cmd_t);

static file_t* mipsu_open_file(const char* s, const char* m, mipsu_ctx_t c) {
    file_t* f = fopen(s, m);

    if (!f) mipsu_exitrv(MIPSU_RESULT_OPEN_FILE, s, c);

    return f;
}

static bool_t mipsu_is_flag(const char* s, const char* f, char c) {
    return s[1] == '-' ? !strcmp(s + 2, f) : (!s[2] && s[1] == c);
}

static bool_t mipsu_handle_flag(const char* s, mipsu_ctx_t* c) {
    mipsu_flag_entry_t e;
    size_t             j;

    for (j = 0; j < mipsu_flagc; ++j) {
        e = mipsu_flag_lut[j];
        if (mipsu_is_flag(s, e.name, e.shrt)) {
            mipsu_set_flag(c, e.bit);
            return true;
        }
    }

    return false;
}

static size_t mipsu_handle_ctx(int32_t argc, const char** argv,
                               const char** args, mipsu_ctx_t* c) {
    size_t i, n = 0;

    c->o = stdout;
    c->f = stdin;

    for (i = 1; i < (size_t)argc; ++i) {

        if (argv[i][0] != '-') {
            args[n++] = argv[i];
            continue;
        }

        if (!argv[i][1]) mipsu_exitr(MIPSU_RESULT_STDIN_CHAR, *c);

        if (mipsu_is_flag(argv[i], "output", 'o')) {
            c->o = mipsu_open_file(argv[++i], "w", *c);
            mipsu_set_flag(c, MIPSU_FLAG_QUIET);
            continue;
        }
        if (mipsu_is_flag(argv[i], "file", 'f')) {
            c->f = mipsu_open_file(argv[++i], "r", *c);
            continue;
        }

        if (mipsu_handle_flag(argv[i], c)) {
            continue;
        }

        args[n++] = argv[i];
    }

    return n;
}

static mipsu_result_t mipsu_handle_cmd(size_t n, const char** args,
                                       mipsu_cmd_t cmd, mipsu_ctx_t c) {
    if (c.f != stdin && n) mipsu_exitr(MIPSU_RESULT_TOO_MANY_ARGS, c);

    switch (n) {
    case 0:
        if (cmd.from_file)
            return cmd.from_file(c);
        else
            return MIPSU_RESULT_FROM_FILE;
    case 1:
        if (cmd.from_arg) return cmd.from_arg(args[0], c);
    default:
        if (cmd.from_args) return cmd.from_args(n, args, c);
    }

    return MIPSU_RESULT_BAD_ARGC;
}

int32_t main(int32_t argc, const char** argv) {
    const char* args[argc];
    size_t      i, n;
    mipsu_ctx_t c = {};

    n = mipsu_handle_ctx(argc, argv, args, &c);

    if (n < 1) {
        puts(mipsu_usage);
        mipsu_exitr(MIPSU_RESULT_MISSING_ARGS, c);
    }

    if (mipsu_is_flag(args[0], "version", 0)) mipsu_version();

    if (mipsu_is_flag(args[0], "help", 'h')) mipsu_help();

    for (i = 0; i < mipsu_cmdc; ++i)
        if (!strcmp(args[0], mipsu_cmdv[i].name))
            mipsu_exitr(mipsu_handle_cmd(n - 1, args + 1, mipsu_cmdv[i], c), c);

    puts(mipsu_usage);
    mipsu_exitrv(MIPSU_RESULT_BAD_CMD, args[0], c);
}
