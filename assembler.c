#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define _LEXER_IMPL
#define _LEXER_KEYWORDS \
  KW_NOP,               \
  KW_PUSH,              \
  KW_DUP,               \
  KW_ADD,               \
  KW_SUB,               \
  KW_MUL,               \
  KW_DIV,               \
  KW_EQ,                \
  KW_JMP,               \
  KW_JZ,                \
  KW_JNZ,               \
  KW_HALT
#define _LEXER_TOKENIZE_NEWLINE
#include "lexer.h"

#define _(string, length) (((uint8_t)length << 4) ^ (string)[0] ^ ((string)[length - 1]))
const token_t KEYWORD_MAP[255] = {
  [_("nop", 3)] = KW_NOP,
  [_("push", 4)] = KW_PUSH,
  [_("dup", 3)] = KW_DUP,
  [_("add", 3)] = KW_ADD,
  [_("sub", 3)] = KW_SUB,
  [_("mul", 3)] = KW_MUL,
  [_("div", 3)] = KW_DIV,
  [_("eq", 2)] = KW_EQ,
  [_("jmp", 3)] = KW_JMP,
  [_("jz", 2)] = KW_JZ,
  [_("jnz", 3)] = KW_JNZ,
  [_("halt", 4)] = KW_HALT,
};

token_t lx_maybe_keyword(const char *str, size_t length)
{
  token_t index = KEYWORD_MAP[_(str, length)];
  return index ? index : TOKEN_IDENTIFIER;
}
#undef _

const size_t KEYWORD_COUNT = sizeof((uint8_t[]){_LEXER_KEYWORDS});
const token_t KEYWORDS[KEYWORD_COUNT] = {_LEXER_KEYWORDS};

#define _PARSER_IMPL
#include "parser.h"

#include "vm.h"
#include "disk.h"

typedef struct {
  Inst* buffer;
  size_t insts;
  size_t instc;
} Ctx;

static inst_t kw_to_inst_t(token_t kw)
{
  assert(kw >= KW_NOP && KW_HALT >= kw);
  if (kw == KW_HALT) return INST_HALT;
  // All keywords are guaranteed to be inserted into
  // the enum in the order that they were defined.
  //
  // The keywords are also defined in the same exact order as `inst_t `is,
  // besides (INST_HALT = 255). This means we can just subtract `KW_NOP`
  // (the first element) and cast to `inst_t`.
  else return kw - KW_NOP;
}

static void ctx_ins(Ctx *c, Inst instruction)
{
  if (c->insts == c->instc) {
    c->buffer = reallocf(c->buffer, c->instc * 2);
    if (c->buffer == NULL) exit(1);
  }
  c->buffer[c->insts++] = instruction;
}

static parse_error_t parse_operand(Parser *p, Value *out)
{
  int64_t sign = 1;
  if (parse_peek(p).type == TOKEN_MINUS) {
    (void)parse_advance(p);
    sign = -1;
  }

  Token t = parse_expect(p, TOKEN_INTEGER);
  if (t.type == TOKEN_ERROR) return p->error;
  assert(t.length != 0);

  Value value = 0;
  for (size_t i = 0; i < t.length; i++)
    value = value * 10 + (t.start[i] - '0');
  *out = value * sign;

  return PARSE_ERR_NONE;
}

static parse_error_t instruction(Ctx *c, Parser *p)
{
  Token token = parse_expect_one_of(p, KEYWORDS, KEYWORD_COUNT);
  if (token.type == TOKEN_ERROR) return p->error;

  Inst instruction = {0};
  instruction.type = kw_to_inst_t(token.type);
  if (VM_INST_HAS_OP[instruction.type])
    parse_operand(p, &instruction.operand);

  Token term = parse_expect(p, TOKEN_NEWLINE);
  if (term.type == TOKEN_ERROR) return p->error;

  ctx_ins(c, instruction);

  return PARSE_ERR_NONE;
}

const char *OUT_FILE_EXT = ".ins";

const char *derive_out_path(const char *inpath)
{
  size_t length = strlen(inpath);
  for (size_t i = length; i != 0;) {
    if (inpath[--i] == '/') break;
    if (inpath[i] != '.') continue;
    length = i; break;
  }
  char *buffer = malloc(length + strlen(OUT_FILE_EXT));
  if (buffer == NULL) exit(1);
  (void)memcpy(buffer, inpath, length);
  (void)memcpy(buffer + length, OUT_FILE_EXT, strlen(OUT_FILE_EXT));
  return buffer;
}

int main(int argc, const char *argv[])
{
  if (argc != 2) {
    fprintf(stderr,
            "Error: expected a path to a file\n"
            "usage: %s <filepath>\n",
            argv[0]);
    return 1;
  }

  size_t nread;
  const char *source = (char *)load_bytes_from_disk(argv[1], &nread);
  Lexer lx = lx_new(source);
  Parser p = {0};
  p.lx = &lx;
  Ctx ctx = {
    .buffer = malloc(128 * sizeof(Inst)),
    .insts = 0,
    .instc = 128,
  };

  for (;;) {
    if (parse_peek(&p).type == TOKEN_EOF) break;
    parse_error_t result = instruction(&ctx, &p);
    if (result != PARSE_ERR_NONE) {
      printf("ERROR: error while compiling instruction: %u\n", result);
      return 1;
    }
  }

  const char *output = derive_out_path(argv[1]);
  save_prog_to_disk(output, ctx.buffer, ctx.insts);
  return 0;
}
