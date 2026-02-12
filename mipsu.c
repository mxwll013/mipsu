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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIPSU_VERSION "0.0.1"

typedef uint32_t               mipsu_word_t;
typedef enum mipsu_fmt         mipsu_fmt_t;
typedef enum mipsu_op_fmt      mipsu_op_fmt_t;
typedef struct mipsu_op_entry  mipsu_op_entry_t;
typedef struct mipsu_reg_entry mipsu_reg_entry_t;
typedef struct mipsu_field     mipsu_field_t;

typedef enum mipsu_exit   mipsu_exit_t;
typedef enum mipsu_result mipsu_result_t;
typedef struct mipsu_cmd  mipsu_cmd_t;

enum mipsu_fmt {
    MIPSU_FMT_R,
    MIPSU_FMT_I,
    MIPSU_FMT_J,
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
    MIPSU_RESULT_UNKNOWN_CMD,
    MIPSU_RESULT_MISSING_ARGS,
    MIPSU_RESULT_TOO_MANY_ARGS,
    MIPSU_RESULT_BAD_FMT,
    MIPSU_RESULT_BAD_BASE,
    MIPSU_RESULT_BAD_HEX,
    MIPSU_RESULT_BAD_BIN,
    MIPSU_RESULT_FIELD_OVERFLOW,
    MIPSU_RESULT_SKIPPED,
};

static const uint8_t mipsu_op_offset = 26;
static const uint8_t mipsu_rs_offset = 21;
static const uint8_t mipsu_rt_offset = 16;
static const uint8_t mipsu_rd_offset = 11;
static const uint8_t mipsu_sh_offset = 6;

static const uint8_t  mipsu_6bit_mask  = 0b00111111;
static const uint8_t  mipsu_5bit_mask  = 0b00011111;
static const uint16_t mipsu_16bit_mask = 0xFFFF;
static const uint32_t mipsu_26bit_mask = 0x03FFFFFF;

static const char* mipsu_fmt_none    = "0x%08X  %-8s\n";
static const char* mipsu_fmt_hex     = "0x%08X  %-8s 0x%08X\n";
static const char* mipsu_fmt_1r      = "0x%08X  %-8s $%-4s\n";
static const char* mipsu_fmt_2r      = "0x%08X  %-8s $%-4s, $%-4s\n";
static const char* mipsu_fmt_3r      = "0x%08X  %-8s $%-4s, $%-4s, $%-4s\n";
static const char* mipsu_fmt_2r_sh   = "0x%08X  %-8s $%-4s, $%-4s, %hhu\n";
static const char* mipsu_fmt_1r_imm  = "0x%08X  %-8s $%-4s, %hi\n";
static const char* mipsu_fmt_2r_imm  = "0x%08X  %-8s $%-4s, $%-4s, %hi\n";
static const char* mipsu_fmt_r_imm_r = "0x%08X  %-8s $%-4s, %hi($%s)\n";
static const char* mipsu_fmt_addr    = "0x%08X  %-8s %u\n";

struct mipsu_reg_entry {
    const char* name;
    const char* num;
};

struct mipsu_op_entry {
    const char*    mnem;
    mipsu_op_fmt_t fmt;
};

struct mipsu_field {
    mipsu_fmt_t fmt;
    uint8_t     op;
    union {
        struct {
            uint8_t rs, rt, rd, sh, fn;
        } r;
        struct {
            uint8_t rs, rt;
            int16_t imm;
        } i;
        struct {
            uint32_t addr;
        } j;
    };
};

struct mipsu_cmd {
    const char* name;
    mipsu_result_t (*no_arg)();
    mipsu_result_t (*one_arg)(const char*);
    mipsu_result_t (*any_arg)(int32_t, const char**);
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

static const mipsu_op_entry_t mipsu_fn_lut[64] = {
    /* Shift */
    [0x00] = {"sll", MIPSU_OP_FMT_RD_RT_SH},

    [0x02] = {"srl", MIPSU_OP_FMT_RD_RT_SH},
    [0x03] = {"sra", MIPSU_OP_FMT_RD_RT_SH},
    [0x04] = {"sllv", MIPSU_OP_FMT_RD_RT_RS},

    [0x06] = {"srlv", MIPSU_OP_FMT_RD_RT_RS},
    [0x07] = {"srav", MIPSU_OP_FMT_RD_RT_RS},

    /* Misc */
    [0x08] = {"jalr", MIPSU_OP_FMT_RS},
    [0x09] = {"jr", MIPSU_OP_FMT_RS},

    [0x0C] = {"syscall", MIPSU_OP_FMT_NONE},
    [0x0D] = {"break", MIPSU_OP_FMT_NONE},

    /* MUL */
    [0x10] = {"mfhi", MIPSU_OP_FMT_RD},
    [0x11] = {"mthi", MIPSU_OP_FMT_RS},
    [0x12] = {"mflo", MIPSU_OP_FMT_RD},
    [0x13] = {"mtlo", MIPSU_OP_FMT_RS},

    [0x18] = {"mult", MIPSU_OP_FMT_RS_RT},
    [0x19] = {"multu", MIPSU_OP_FMT_RS_RT},
    [0x1A] = {"div", MIPSU_OP_FMT_RS_RT},
    [0x1B] = {"divu", MIPSU_OP_FMT_RS_RT},

    /* ALU */
    [0x20] = {"add", MIPSU_OP_FMT_RD_RS_RT},
    [0x21] = {"addu", MIPSU_OP_FMT_RD_RS_RT},
    [0x22] = {"sub", MIPSU_OP_FMT_RD_RS_RT},
    [0x23] = {"subu", MIPSU_OP_FMT_RD_RS_RT},
    [0x24] = {"and", MIPSU_OP_FMT_RD_RS_RT},
    [0x25] = {"or", MIPSU_OP_FMT_RD_RS_RT},
    [0x26] = {"xor", MIPSU_OP_FMT_RD_RS_RT},
    [0x27] = {"nor", MIPSU_OP_FMT_RD_RS_RT},

    [0x2A] = {"slt", MIPSU_OP_FMT_RD_RS_RT},
    [0x2B] = {"sltu", MIPSU_OP_FMT_RD_RS_RT},
};

static const mipsu_op_entry_t mipsu_op_lut[64] = {
    /* Jump */
    [0x02] = {"j", MIPSU_OP_FMT_ADDR},
    [0x03] = {"jal", MIPSU_OP_FMT_ADDR},

    /* Branch */
    [0x04] = {"beq", MIPSU_OP_FMT_RS_RT_IMM},
    [0x05] = {"bne", MIPSU_OP_FMT_RS_RT_IMM},
    [0x06] = {"blez", MIPSU_OP_FMT_RS_IMM},
    [0x07] = {"bgtz", MIPSU_OP_FMT_RS_IMM},

    /* ALU */
    [0x08] = {"addi", MIPSU_OP_FMT_RT_RS_IMM},
    [0x09] = {"addiu", MIPSU_OP_FMT_RT_RS_IMM},
    [0x0C] = {"andi", MIPSU_OP_FMT_RT_RS_IMM},
    [0x0D] = {"ori", MIPSU_OP_FMT_RT_RS_IMM},
    [0x0F] = {"lui", MIPSU_OP_FMT_RT_IMM},

    /* MEM */
    [0x20] = {"lb", MIPSU_OP_FMT_RT_IMM_RS},
    [0x21] = {"lh", MIPSU_OP_FMT_RT_IMM_RS},

    [0x23] = {"lw", MIPSU_OP_FMT_RT_IMM_RS},

    [0x24] = {"lbu", MIPSU_OP_FMT_RT_IMM_RS},
    [0x25] = {"lhu", MIPSU_OP_FMT_RT_IMM_RS},

    [0x28] = {"sb", MIPSU_OP_FMT_RT_IMM_RS},
    [0x29] = {"sh", MIPSU_OP_FMT_RT_IMM_RS},

    [0x2B] = {"sw", MIPSU_OP_FMT_RT_IMM_RS},
};

static const int8_t mipsu_hex_lut[256] = {
    ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,  ['5'] = 5,
    ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,

    ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
    ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
};

static const char* mipsu_err_msg_lut[] = {
    [MIPSU_EXIT_OK]       = "ok",
    [MIPSU_EXIT_USAGE]    = "bad usage",
    [MIPSU_EXIT_PARSE]    = "parse error",
    [MIPSU_EXIT_INTERNAL] = "internal error",
};

static const char* mipsu_res_msg_lut[] = {
    [MIPSU_RESULT_OK]             = "ok",
    [MIPSU_RESULT_UNKNOWN_CMD]    = "unknown command",
    [MIPSU_RESULT_MISSING_ARGS]   = "missing argument(s)",
    [MIPSU_RESULT_TOO_MANY_ARGS]  = "too many arguments",
    [MIPSU_RESULT_BAD_FMT]        = "bad format",
    [MIPSU_RESULT_BAD_BASE]       = "bad base",
    [MIPSU_RESULT_BAD_HEX]        = "bad hex",
    [MIPSU_RESULT_BAD_BIN]        = "bad binary",
    [MIPSU_RESULT_FIELD_OVERFLOW] = "field overflow",
    [MIPSU_RESULT_SKIPPED]        = "skipped instruction(s)",
};

static const mipsu_exit_t mipsu_err_lut[] = {
    [MIPSU_RESULT_OK] = MIPSU_EXIT_OK,

    [MIPSU_RESULT_UNKNOWN_CMD]   = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_MISSING_ARGS]  = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_TOO_MANY_ARGS] = MIPSU_EXIT_USAGE,

    [MIPSU_RESULT_BAD_FMT]        = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_BASE]       = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_HEX]        = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_BAD_BIN]        = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_FIELD_OVERFLOW] = MIPSU_EXIT_PARSE,
    [MIPSU_RESULT_SKIPPED]        = MIPSU_EXIT_PARSE,
};

static const char* mipsu_usage =

    "\n"
    "usage:\n"
    "  mipsu decode <hex | bin>\n"
    "  mipsu disasm [hex | bin]\n"
    "  mipsu encode -R <rs> <rt> <rd> <sh> <fn>\n"
    "  mipsu encode -I <op> <rs> <rt> <imm>\n"
    "  mipsu encode -J <op> <addr>\n"
    "  mipsu --version\n"
    "  mipsu --help | -h\n"
    "\n"
    "commands:\n"
    "  decode  32bit word -> bitfield\n"
    "  disasm  32bit word -> assembly\n"
    "  encode  bitfield -> 32bit word\n";

static char mipsu_chr_buff[2048];

static void mipsu_cerr(const char* msg, const char* c, const char* v) {
    if (v)
        fprintf(stderr, "\033[%smmipsu\033[0m: %s. '%s'\n", c, msg, v);
    else
        fprintf(stderr, "\033[%smmipsu\033[0m: %s.\n", c, msg);
}

static void mipsu_err(const char* msg) { mipsu_cerr(msg, "31", NULL); }
static void mipsu_errv(const char* msg, const char* v) {
    mipsu_cerr(msg, "31", v);
}
/* static void mipsu_wrn(const char* msg) { mipsu_cerr(msg, "34", NULL); } */
static void mipsu_wrnv(const char* msg, const char* v) {
    mipsu_cerr(msg, "34", v);
}

static void mipsu_exit(mipsu_exit_t c) {
    if (c) mipsu_err(mipsu_err_msg_lut[c]);
    exit(c);
}

static void mipsu_exitr(mipsu_result_t r) {
    if (r) mipsu_err(mipsu_res_msg_lut[r]);
    mipsu_exit(mipsu_err_lut[r]);
}

static void mipsu_exitrv(mipsu_result_t r, const char* v) {
    if (r) mipsu_errv(mipsu_res_msg_lut[r], v);
    mipsu_exit(mipsu_err_lut[r]);
}

/* === Parsing === */

static int mipsu_bin_spec(char c) { return c == 'B' || c == 'b'; }
static int mipsu_hex_spec(char c) { return c == 'X' || c == 'x'; }
static int mipsu_bin_valid(const char* s, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i)
        if (s[i] != '0' && s[i] != '1') return 0;
    return 1;
}
static int mipsu_hex_valid(const char* s, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i)
        switch (s[i]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            continue;
        default:
            return 0;
        }

    return 1;
}

static mipsu_word_t mipsu_get_word(const char* s, size_t n, uint8_t sh) {
    size_t       i;
    mipsu_word_t v = 0;

    for (i = 0; i < n; ++i)
        v = (v << sh) | (mipsu_word_t)mipsu_hex_lut[(size_t)s[i]];

    return v;
}

static mipsu_result_t mipsu_parse_word(const char* s, mipsu_word_t* out,
                                       uint8_t b) {
    size_t  n  = b;
    uint8_t sh = 1;

    if (s[0] != '0') return MIPSU_RESULT_BAD_BASE;

    if (mipsu_hex_spec(s[1])) {
        n  = (b + 3) / 4;
        sh = 4;
        if (!mipsu_hex_valid(s + 2, n)) return MIPSU_RESULT_BAD_HEX;
    } else if (mipsu_bin_spec(s[1])) {
        if (!mipsu_bin_valid(s + 2, n)) return MIPSU_RESULT_BAD_BIN;
    } else
        return MIPSU_RESULT_BAD_BASE;

    *out = mipsu_get_word(s + 2, n, sh);

    if (*out >= 2u << b) return MIPSU_RESULT_FIELD_OVERFLOW;

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_parse_fmt(const char* s, mipsu_fmt_t* out) {
    if (strlen(s) > 2) return MIPSU_RESULT_BAD_FMT;
    if (s[0] == '-') ++s;

    switch (s[0]) {
    case 'R':
    case 'r':
        *out = MIPSU_FMT_R;
        break;
    case 'I':
    case 'i':
        *out = MIPSU_FMT_I;
        break;
    case 'J':
    case 'j':
        *out = MIPSU_FMT_J;
        break;
    default:
        mipsu_errv("unknown format", s);
        return MIPSU_RESULT_BAD_FMT;
    }

    if (s[1]) return MIPSU_RESULT_BAD_FMT;

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_parse_field_hlpr(const char* s, const char* msg,
                                             uint8_t* f, uint8_t n) {
    mipsu_result_t r;
    mipsu_word_t   w;

    r  = mipsu_parse_word(s, &w, n);
    *f = w;

    if (r) mipsu_errv(msg, s);

    return r;
}

static mipsu_result_t mipsu_parse_r_field(int32_t argc, const char** argv,
                                          mipsu_field_t* out) {
    mipsu_result_t r;

    if (argc < 6) return MIPSU_RESULT_MISSING_ARGS;
    if (argc > 6) return MIPSU_RESULT_TOO_MANY_ARGS;

    out->fmt = MIPSU_FMT_R;
    out->op  = 0;

    r = mipsu_parse_field_hlpr(argv[1], "in <rs>", &out->r.rs, 5);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[2], "in <rt>", &out->r.rt, 5);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[3], "in <rd>", &out->r.rd, 5);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[4], "in <sh>", &out->r.sh, 5);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[5], "in <fn>", &out->r.fn, 6);

    return r;
}

static mipsu_result_t mipsu_parse_i_field(int32_t argc, const char** argv,
                                          mipsu_field_t* out) {
    mipsu_result_t r;
    mipsu_word_t   w;

    if (argc < 5) return MIPSU_RESULT_MISSING_ARGS;
    if (argc > 5) return MIPSU_RESULT_TOO_MANY_ARGS;

    out->fmt = MIPSU_FMT_I;

    r = mipsu_parse_field_hlpr(argv[1], "in <op>", &out->op, 6);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[2], "in <rs>", &out->i.rs, 5);
    if (r) return r;

    r = mipsu_parse_field_hlpr(argv[3], "in <rt>", &out->i.rt, 5);
    if (r) return r;

    r          = mipsu_parse_word(argv[4], &w, 16);
    out->i.imm = w;
    if (r) mipsu_errv("in <imm>", argv[4]);

    return r;
}

static mipsu_result_t mipsu_parse_j_field(int32_t argc, const char** argv,
                                          mipsu_field_t* out) {
    mipsu_result_t r;
    mipsu_word_t   w;

    if (argc < 3) return MIPSU_RESULT_MISSING_ARGS;
    if (argc > 3) return MIPSU_RESULT_TOO_MANY_ARGS;

    out->fmt = MIPSU_FMT_I;

    r = mipsu_parse_field_hlpr(argv[1], "in <op>", &out->op, 6);
    if (r) return r;

    r         = mipsu_parse_word(argv[2], &w, 26);
    out->i.rs = w;
    if (r) mipsu_errv("in <addr>", argv[2]);

    return r;
}

static mipsu_result_t mipsu_parse_field(int32_t argc, const char** argv,
                                        mipsu_field_t* out) {
    mipsu_fmt_t    fmt;
    mipsu_result_t r;

    if (argc < 1) return MIPSU_RESULT_MISSING_ARGS;

    r = mipsu_parse_fmt(argv[0], &fmt);

    if (r) return r;

    switch (fmt) {
    case MIPSU_FMT_R:
        return mipsu_parse_r_field(argc, argv, out);
    case MIPSU_FMT_I:
        return mipsu_parse_i_field(argc, argv, out);
    case MIPSU_FMT_J:
        return mipsu_parse_j_field(argc, argv, out);
    }
}

/* === Formatting === */

static size_t mipsu_fmt_r_field(mipsu_word_t w, mipsu_field_t f, char* b) {
    const char* rs = mipsu_reg_lut[f.r.rs].name;
    const char* rt = mipsu_reg_lut[f.r.rt].name;
    const char* rd = mipsu_reg_lut[f.r.rd].name;
    const char* fn = mipsu_fn_lut[f.r.fn].mnem;

    size_t o = 0;

    o += sprintf(b + o, "hex:   0x%08X\n", w);
    o += sprintf(b + o, "type:  R\n");
    o += sprintf(b + o, "--------\n");
    o += sprintf(b + o, "rs:  0x%02hhX  ($%s)\n", f.r.rs, rs);
    o += sprintf(b + o, "rt:  0x%02hhX  ($%s)\n", f.r.rt, rt);
    o += sprintf(b + o, "rd:  0x%02hhX  ($%s)\n", f.r.rd, rd);
    o += sprintf(b + o, "sh:  0x%02hhX  (%hhu)\n", f.r.sh, f.r.sh);
    o += sprintf(b + o, "fn:  0x%02hhX  (%s)\n", f.r.fn, fn);

    return o;
}

static size_t mipsu_fmt_i_field(mipsu_word_t w, mipsu_field_t f, char* b) {
    const char* rs = mipsu_reg_lut[f.r.rs].name;
    const char* rt = mipsu_reg_lut[f.r.rt].name;
    const char* op = mipsu_op_lut[f.op].mnem;

    size_t o = 0;

    o += sprintf(b + o, "hex:   0x%08X\n", w);
    o += sprintf(b + o, "type:  I\n");
    o += sprintf(b + o, "--------\n");
    o += sprintf(b + o, "op:   0x%02hhX    (%s)\n", f.op, op);
    o += sprintf(b + o, "rs:   0x%02hhX    ($%s)\n", f.i.rs, rs);
    o += sprintf(b + o, "rt:   0x%02hhX    ($%s)\n", f.i.rt, rt);
    o += sprintf(b + o, "imm:  0x%04hX  (%hi)\n", f.i.imm, f.i.imm);

    return o;
}

static size_t mipsu_fmt_j_field(mipsu_word_t w, mipsu_field_t f, char* b) {
    const char* op = mipsu_op_lut[f.op].mnem;

    size_t o = 0;

    o += sprintf(b + o, "hex:   0x%08X\n", w);
    o += sprintf(b + o, "type:  J\n");
    o += sprintf(b + o, "--------\n");
    o += sprintf(b + o, "op:    0x%02hhX      (%s)\n", f.op, op);
    o += sprintf(b + o, "addr:  0x%08X  (%u)\n", f.j.addr, f.j.addr);

    return o;
}

static size_t mipsu_fmt_field(mipsu_word_t w, mipsu_field_t f, char* b) {
    switch (f.fmt) {
    case MIPSU_FMT_R:
        return mipsu_fmt_r_field(w, f, b);
    case MIPSU_FMT_I:
        return mipsu_fmt_i_field(w, f, b);
    case MIPSU_FMT_J:
        return mipsu_fmt_j_field(w, f, b);
    }
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
        f.fmt  = MIPSU_FMT_R;
        f.r.rs = mipsu_rs(w);
        f.r.rt = mipsu_rt(w);
        f.r.rd = mipsu_rd(w);
        f.r.sh = mipsu_sh(w);
        f.r.fn = mipsu_fn(w);
    } else if (f.op == 2 || f.op == 3) {
        f.fmt    = MIPSU_FMT_J;
        f.j.addr = mipsu_addr(w);
    } else {
        f.fmt   = MIPSU_FMT_I;
        f.i.rs  = mipsu_rs(w);
        f.i.rt  = mipsu_rt(w);
        f.i.imm = mipsu_imm(w);
    }

    return f;
}

static void mipsu_dump_decoded(mipsu_word_t w) {

    mipsu_field_t f = mipsu_decode(w);

    mipsu_fmt_field(w, f, mipsu_chr_buff);

    fputs(mipsu_chr_buff, stdout);
}

/* === Encoding === */

static mipsu_word_t mipsu_encode(mipsu_field_t f) {

    switch (f.fmt) {
    case MIPSU_FMT_R:
        return (
            ((mipsu_word_t)f.op << mipsu_op_offset) |
            ((mipsu_word_t)f.r.rs << mipsu_rs_offset) |
            ((mipsu_word_t)f.r.rt << mipsu_rt_offset) |
            ((mipsu_word_t)f.r.rd << mipsu_rd_offset) |
            ((mipsu_word_t)f.r.sh << mipsu_sh_offset | (mipsu_word_t)f.r.fn));
    case MIPSU_FMT_I:
        return (((mipsu_word_t)f.op << mipsu_op_offset) |
                ((mipsu_word_t)f.i.rs << mipsu_rs_offset) |
                ((mipsu_word_t)f.i.rt << mipsu_rt_offset) |
                ((mipsu_word_t)f.i.imm & mipsu_16bit_mask));
    case MIPSU_FMT_J:
        return (((mipsu_word_t)f.op << mipsu_op_offset) |
                (mipsu_word_t)f.j.addr);
    }
}

static void mipsu_dump_encoded(mipsu_field_t f) {
    mipsu_word_t w = mipsu_encode(f);

    mipsu_fmt_field(w, f, mipsu_chr_buff);

    fputs(mipsu_chr_buff, stdout);
}

/* === Disassembly === */

static size_t mipsu_disasm(mipsu_word_t w, mipsu_field_t f, char* b) {
    const char* rd = mipsu_reg_lut[f.r.rd].name;
    const char* rs = mipsu_reg_lut[f.r.rs].name;
    const char* rt = mipsu_reg_lut[f.r.rt].name;

    mipsu_op_entry_t e;

    if (f.fmt == MIPSU_FMT_R)
        e = mipsu_fn_lut[f.r.fn];
    else
        e = mipsu_op_lut[f.op];

    switch (e.fmt) {
    case MIPSU_OP_FMT_UNKNOWN:
        return sprintf(b, mipsu_fmt_hex, w, ".word", w);
    case MIPSU_OP_FMT_NONE:
        return sprintf(b, mipsu_fmt_none, w, e.mnem);

    case MIPSU_OP_FMT_RS:
        return sprintf(b, mipsu_fmt_1r, w, e.mnem, rs);
    case MIPSU_OP_FMT_RD:
        return sprintf(b, mipsu_fmt_1r, w, e.mnem, rd);
    case MIPSU_OP_FMT_RS_RT:
        return sprintf(b, mipsu_fmt_2r, w, e.mnem, rs, rd);
    case MIPSU_OP_FMT_RD_RT_RS:
        return sprintf(b, mipsu_fmt_3r, w, e.mnem, rd, rt, rs);
    case MIPSU_OP_FMT_RD_RT_SH:
        return sprintf(b, mipsu_fmt_2r_sh, w, e.mnem, rd, rt, f.r.sh);
    case MIPSU_OP_FMT_RD_RS_RT:
        return sprintf(b, mipsu_fmt_3r, w, e.mnem, rd, rs, rt);

    case MIPSU_OP_FMT_RS_IMM:
        return sprintf(b, mipsu_fmt_1r_imm, w, e.mnem, rs, f.i.imm);
    case MIPSU_OP_FMT_RT_IMM:
        return sprintf(b, mipsu_fmt_1r_imm, w, e.mnem, rt, f.i.imm);
    case MIPSU_OP_FMT_RT_IMM_RS:
        return sprintf(b, mipsu_fmt_r_imm_r, w, e.mnem, rt, f.i.imm, rs);
    case MIPSU_OP_FMT_RT_RS_IMM:
        return sprintf(b, mipsu_fmt_2r_imm, w, e.mnem, rt, rs, f.i.imm);
    case MIPSU_OP_FMT_RS_RT_IMM:
        return sprintf(b, mipsu_fmt_2r_imm, w, e.mnem, rs, rt, f.i.imm);

    case MIPSU_OP_FMT_ADDR:
        return sprintf(b, mipsu_fmt_addr, w, e.mnem, f.j.addr);
    }
}

static void mipsu_dump_disasm(mipsu_word_t w) {
    mipsu_field_t f = mipsu_decode(w);

    mipsu_disasm(w, f, mipsu_chr_buff);

    fputs(mipsu_chr_buff, stdout);
}

/* === Command Line Interface === */

static mipsu_result_t mipsu_arg_decode(const char* s) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_word(s, &w, 32);

    if (r) return r;

    mipsu_dump_decoded(w);

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_args_encode(int32_t argc, const char** argv) {

    mipsu_field_t  f;
    mipsu_result_t r;

    r = mipsu_parse_field(argc, argv, &f);

    if (r) return r;

    mipsu_dump_encoded(f);

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_stdin_disasm() {
    mipsu_word_t w;
    user_size_t  s = 0;

    while (fgets(mipsu_chr_buff, sizeof(mipsu_chr_buff), stdin)) {

        if (mipsu_parse_word(mipsu_chr_buff, &w, 32)) {
            mipsu_wrnv("skipping instruction", mipsu_chr_buff);
            ++s;
            continue;
        }

        mipsu_dump_disasm(w);
    }

    return s ? MIPSU_RESULT_SKIPPED : MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_arg_disasm(const char* s) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_word(s, &w, 32);

    if (r) return r;

    mipsu_dump_disasm(w);

    return MIPSU_RESULT_OK;
}

static void mipsu_version() {
    printf("mipsu %s\n", MIPSU_VERSION);
    mipsu_exit(MIPSU_EXIT_OK);
}

static void mipsu_help() {
    puts(mipsu_usage);
    mipsu_exit(MIPSU_EXIT_OK);
}

static const mipsu_cmd_t mipsu_cmdv[3] = {
    {
        "decode",
        NULL,
        mipsu_arg_decode,
        NULL,
    },
    {
        "disasm",
        mipsu_stdin_disasm,
        mipsu_arg_disasm,
        NULL,
    },
    {
        "encode",
        NULL,
        NULL,
        mipsu_args_encode,
    },
};

static void mipsu_handle(int32_t argc, const char** argv, mipsu_cmd_t cmd) {

    if (argc == 0 && cmd.no_arg) mipsu_exitr(cmd.no_arg());
    if (argc == 1 && cmd.one_arg) mipsu_exitr(cmd.one_arg(argv[0]));

    if (cmd.any_arg) mipsu_exitr(cmd.any_arg(argc, argv));

    mipsu_exit(MIPSU_EXIT_USAGE);
}

int32_t main(int32_t argc, const char** argv) {
    size_t i;

    if (argc < 2) {
        puts(mipsu_usage);
        mipsu_exitr(MIPSU_RESULT_MISSING_ARGS);
    }

    if (!strcmp(argv[1], "--version")) mipsu_version();

    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) mipsu_help();

    for (i = 0; i < sizeof(mipsu_cmdv) / sizeof(mipsu_cmd_t); ++i)
        if (!strcmp(argv[1], mipsu_cmdv[i].name))
            mipsu_handle(argc - 2, argv + 2, mipsu_cmdv[i]);

    puts(mipsu_usage);
    mipsu_exitrv(MIPSU_RESULT_UNKNOWN_CMD, argv[1]);
}
