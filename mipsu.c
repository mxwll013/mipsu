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

#define MIPSU_VERSION "0.0.0"

/* Exit codes */
#define MIPSU_EXIT_OK       0
#define MIPSU_EXIT_USAGE    1
#define MIPSU_EXIT_PARSE    2
#define MIPSU_EXIT_INTERNAL 3

/* Result codes */
#define MIPSU_RESULT_OK            0
#define MIPSU_RESULT_UNKNOWN_CMD   1
#define MIPSU_RESULT_TOO_MANY_ARGS 2
#define MIPSU_RESULT_BAD_HEX       3

/* MIPS Types */
typedef uint32_t               mipsu_word_t;
typedef enum mipsu_fmt         mipsu_fmt_t;
typedef enum mipsu_op_fmt      mipsu_op_fmt_t;
typedef struct mipsu_op_entry  mipsu_op_entry_t;
typedef struct mipsu_reg_entry mipsu_reg_entry_t;
typedef struct mipsu_field     mipsu_field_t;

/* CLI Types */
typedef int32_t          mipsu_exit_t;
typedef int32_t          mipsu_result_t;
typedef struct mipsu_cmd mipsu_cmd_t;

/* MIPS Enums */
enum mipsu_fmt {
    MIPSU_FMT_R,
    MIPSU_FMT_I,
    MIPSU_FMT_J,
};

enum mipsu_op_fmt {
    MIPSU_OP_FMT_NONE,

    MIPSU_OP_FMT_RD_RS_RT,

    MIPSU_OP_FMT_RT_RS_IMM,
    MIPSU_OP_FMT_RT_IMM_RS,
    MIPSU_OP_FMT_RS_RT_IMM,

    MIPSU_OP_FMT_ADDR,
};

/* MIPS Entries */
struct mipsu_reg_entry {
    const char* name;
    const char* num;
};

struct mipsu_op_entry {
    const char*    mnem;
    mipsu_op_fmt_t fmt;
};

/* MIPS Bitfield */
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

/* CLI command */
struct mipsu_cmd {
    const char* name;
    mipsu_result_t (*stream)();
    mipsu_result_t (*single)(const char*);
};

/* MIPS Look up tables */
static mipsu_reg_entry_t mipsu_reg_lut[] = {
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

static mipsu_op_entry_t mipsu_fn_lut[] = {
    [0x20] = {"add", MIPSU_OP_FMT_RD_RS_RT},
    [0x21] = {"addu", MIPSU_OP_FMT_RD_RS_RT},
    [0x22] = {"sub", MIPSU_OP_FMT_RD_RS_RT},
    [0x23] = {"subu", MIPSU_OP_FMT_RD_RS_RT},
    [0x24] = {"and", MIPSU_OP_FMT_RD_RS_RT},
    [0x25] = {"or", MIPSU_OP_FMT_RD_RS_RT},
    [0x2A] = {"slt", MIPSU_OP_FMT_RD_RS_RT},
};

static mipsu_op_entry_t mipsu_op_lut[] = {
    [0x02] = {"j", MIPSU_OP_FMT_ADDR},
    [0x03] = {"jal", MIPSU_OP_FMT_ADDR},

    [0x04] = {"beq", MIPSU_OP_FMT_RS_RT_IMM},
    [0x05] = {"bne", MIPSU_OP_FMT_RS_RT_IMM},

    [0x08] = {"addi", MIPSU_OP_FMT_RT_RS_IMM},
    [0x09] = {"addiu", MIPSU_OP_FMT_RT_RS_IMM},
    [0x0C] = {"andi", MIPSU_OP_FMT_RT_RS_IMM},
    [0x0D] = {"ori", MIPSU_OP_FMT_RT_RS_IMM},

    [0x23] = {"lw", MIPSU_OP_FMT_RT_IMM_RS},
    [0x2B] = {"sw", MIPSU_OP_FMT_RT_IMM_RS},
};

static const int8_t mipsu_hex_lut[256] = {
    ['0'] = 0,  ['1'] = 1,  ['2'] = 2,  ['3'] = 3,  ['4'] = 4,  ['5'] = 5,
    ['6'] = 6,  ['7'] = 7,  ['8'] = 8,  ['9'] = 9,

    ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
    ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
};

static char mipsu_chr_buff[2048];

/* CLI Look up tables */
static const char* mipsu_err_msg_lut[] = {
    [MIPSU_EXIT_OK]       = "ok",
    [MIPSU_EXIT_USAGE]    = "bad usage",
    [MIPSU_EXIT_PARSE]    = "parse error",
    [MIPSU_EXIT_INTERNAL] = "internal error",
};

static const char* mipsu_res_msg_lut[] = {
    [MIPSU_RESULT_OK]            = "ok",
    [MIPSU_RESULT_UNKNOWN_CMD]   = "unknown command",
    [MIPSU_RESULT_TOO_MANY_ARGS] = "too many arguments",
    [MIPSU_RESULT_BAD_HEX]       = "bad hex",
};

static const mipsu_exit_t mipsu_err_lut[] = {
    [MIPSU_RESULT_OK]            = MIPSU_EXIT_OK,
    [MIPSU_RESULT_UNKNOWN_CMD]   = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_TOO_MANY_ARGS] = MIPSU_EXIT_USAGE,
    [MIPSU_RESULT_BAD_HEX]       = MIPSU_EXIT_PARSE,
};

/* CLI Usage string */
static const char* mipsu_usage =

    "\n"
    "usage: mipsu <command> [options] [file]\n"
    "\n"
    "commands:\n"
    "  decode  Decode instructions\n"
    "  disasm  Disassemble\n";

/* CLI helpers */
static void mipsu_err(const char* msg) { fprintf(stderr, "mipsu: %s.\n", msg); }

static void mipsu_exit(mipsu_exit_t c) {
    if (c) mipsu_err(mipsu_err_msg_lut[c]);
    exit(c);
}

static void mipsu_exitr(mipsu_result_t r) {
    if (r) mipsu_err(mipsu_res_msg_lut[r]);
    mipsu_exit(mipsu_err_lut[r]);
}

/* === Parsing === */

static mipsu_word_t mipsu_hex_to_word(const char* s) {
    size_t       i;
    mipsu_word_t v;

    for (i = 0; i < 8; ++i)
        v = (v << 4) | (mipsu_word_t)mipsu_hex_lut[(size_t)s[i]];

    return v;
}

static int mipsu_valid_hex(char c) {
    switch (c) {
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
        return 1;
    default:
        return 0;
    }
}

static mipsu_result_t mipsu_parse_hex(const char* s, mipsu_word_t* out) {
    size_t i;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;

    for (i = 0; i < 8; ++i)
        if (!mipsu_valid_hex(s[i])) return MIPSU_RESULT_BAD_HEX;

    *out = mipsu_hex_to_word(s);

    return MIPSU_RESULT_OK;
}

/* === Decoding === */

static uint8_t  mipsu_op(mipsu_word_t w) { return (w >> 26) & 0x3F; }
static uint8_t  mipsu_rs(mipsu_word_t w) { return (w >> 21) & 0x1F; }
static uint8_t  mipsu_rt(mipsu_word_t w) { return (w >> 16) & 0x1F; }
static uint8_t  mipsu_rd(mipsu_word_t w) { return (w >> 11) & 0x1F; }
static uint8_t  mipsu_sh(mipsu_word_t w) { return (w >> 6) & 0x1F; }
static uint8_t  mipsu_fn(mipsu_word_t w) { return w & 0x3F; }
static int16_t  mipsu_imm(mipsu_word_t w) { return w & 0xFFFF; }
static uint32_t mipsu_addr(mipsu_word_t w) { return w & 0x03FFFFFF; }

static mipsu_field_t mipsu_word_to_field(mipsu_word_t w) {
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

static size_t mipsu_fmt_r_decoded(mipsu_word_t w, mipsu_field_t f, char* b) {
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

static size_t mipsu_fmt_i_decoded(mipsu_word_t w, mipsu_field_t f, char* b) {
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

static size_t mipsu_fmt_j_decoded(mipsu_word_t w, mipsu_field_t f, char* b) {
    const char* op = mipsu_op_lut[f.op].mnem;

    size_t o = 0;

    o += sprintf(b + o, "hex:   0x%08X\n", w);
    o += sprintf(b + o, "type:  J\n");
    o += sprintf(b + o, "--------\n");
    o += sprintf(b + o, "op:    0x%02hhX      (%s)\n", f.op, op);
    o += sprintf(b + o, "addr:  0x%08X  (%u)\n", f.j.addr, f.j.addr);

    return o;
}

static size_t mipsu_fmt_decoded(mipsu_word_t w, mipsu_field_t f, char* b) {
    switch (f.fmt) {
    case MIPSU_FMT_R:
        return mipsu_fmt_r_decoded(w, f, b);
    case MIPSU_FMT_I:
        return mipsu_fmt_i_decoded(w, f, b);
    case MIPSU_FMT_J:
        return mipsu_fmt_j_decoded(w, f, b);
    }
}

static void mipsu_dump_decoded(mipsu_word_t w) {

    mipsu_field_t f = mipsu_word_to_field(w);

    mipsu_fmt_decoded(w, f, mipsu_chr_buff);

    printf("%s", mipsu_chr_buff);
}

/* === Disassembling === */

static size_t mipsu_fmt_disasm(mipsu_word_t w, mipsu_field_t f, char* b) {
    mipsu_op_entry_t e;

    if (f.fmt == MIPSU_FMT_R)
        e = mipsu_fn_lut[f.r.fn];
    else
        e = mipsu_op_lut[f.op];

    switch (e.fmt) {
    case MIPSU_OP_FMT_RD_RS_RT:
        return sprintf(b,
                       "0x%08X  %-6s $%s, &%s, $%s\n",
                       w,
                       e.mnem,
                       mipsu_reg_lut[f.r.rd].name,
                       mipsu_reg_lut[f.r.rs].name,
                       mipsu_reg_lut[f.r.rt].name);

    case MIPSU_OP_FMT_RT_RS_IMM:
        return sprintf(b,
                       "0x%08X  %-6s $%s, $%s, %hi\n",
                       w,
                       e.mnem,
                       mipsu_reg_lut[f.i.rt].name,
                       mipsu_reg_lut[f.i.rs].name,
                       f.i.imm);
    case MIPSU_OP_FMT_RT_IMM_RS:
        return sprintf(b,
                       "0x%08X  %-6s $%s, %hi($%s)\n",
                       w,
                       e.mnem,
                       mipsu_reg_lut[f.i.rt].name,
                       f.i.imm,
                       mipsu_reg_lut[f.i.rs].name);
    case MIPSU_OP_FMT_RS_RT_IMM:
        return sprintf(b,
                       "0x%08X  %-6s $%s, $%s, %hi\n",
                       w,
                       e.mnem,
                       mipsu_reg_lut[f.i.rs].name,
                       mipsu_reg_lut[f.i.rt].name,
                       f.i.imm);

    case MIPSU_OP_FMT_ADDR:
        return sprintf(b, "0x%08X  %-6s %u\n", w, e.mnem, f.j.addr);

    default:
        return sprintf(b, "0x%08X  %-6s 0x%08X\n", w, ".word", w);
    }
}

static void mipsu_dump_disasm(mipsu_word_t w) {
    mipsu_field_t f = mipsu_word_to_field(w);

    mipsu_fmt_disasm(w, f, mipsu_chr_buff);

    printf("%s", mipsu_chr_buff);
}

/* === Command Line Interface === */

static mipsu_result_t mipsu_stream_decode() {
    mipsu_word_t   w;
    mipsu_result_t r = MIPSU_RESULT_OK;

    while (fgets(mipsu_chr_buff, sizeof(mipsu_chr_buff), stdin)) {

        if (mipsu_parse_hex(mipsu_chr_buff, &w)) {
            mipsu_err("bad hex");
            r = MIPSU_RESULT_BAD_HEX;
            continue;
        }

        mipsu_dump_decoded(w);
    }

    return r;
}

static mipsu_result_t mipsu_single_decode(const char* s) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_hex(s, &w);

    if (r) return r;

    mipsu_dump_decoded(w);

    return MIPSU_RESULT_OK;
}

static mipsu_result_t mipsu_stream_disasm() {
    mipsu_word_t   w;
    mipsu_result_t r = MIPSU_RESULT_OK;

    while (fgets(mipsu_chr_buff, sizeof(mipsu_chr_buff), stdin)) {

        if (mipsu_parse_hex(mipsu_chr_buff, &w)) {
            mipsu_err("bad hex");
            r = MIPSU_RESULT_BAD_HEX;
            continue;
        }

        mipsu_dump_disasm(w);
    }

    return r;
}

static mipsu_result_t mipsu_single_disasm(const char* s) {
    mipsu_word_t   w;
    mipsu_result_t r;

    r = mipsu_parse_hex(s, &w);

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

static const size_t      mipsu_cmdc   = 2;
static const mipsu_cmd_t mipsu_cmdv[] = {
    {
        "decode",
        mipsu_stream_decode,
        mipsu_single_decode,
    },
    {
        "disasm",
        mipsu_stream_disasm,
        mipsu_single_disasm,
    },
};

static void mipsu_handle(int32_t argc, const char** argv, mipsu_cmd_t cmd) {
    mipsu_result_t r;
    switch (argc) {
    case 0:
        r = cmd.stream();
        break;
    case 1:
        r = cmd.single(argv[0]);
        break;
    default:
        r = MIPSU_RESULT_TOO_MANY_ARGS;
    }

    mipsu_exitr(r);
}

int32_t main(int32_t argc, const char** argv) {
    size_t i;

    if (argc < 2) {
        puts(mipsu_usage);
        mipsu_exit(MIPSU_EXIT_USAGE);
    }

    if (!strcmp(argv[1], "--version")) mipsu_version();

    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) mipsu_help();

    for (i = 0; i < mipsu_cmdc; ++i)
        if (!strcmp(argv[1], mipsu_cmdv[i].name))
            mipsu_handle(argc - 2, argv + 2, mipsu_cmdv[i]);

    puts(mipsu_usage);
    mipsu_exitr(MIPSU_RESULT_UNKNOWN_CMD);
}
