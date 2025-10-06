#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STB_DS_IMPLEMENTATION
#include "include/stb_ds.h"

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
typedef struct { const char *name; token_t repr; } Kw;
const Kw KEYWORD_MAP[255] = {
  [_("nop", 3)] = {"nop", KW_NOP},
  [_("push", 4)] = {"push", KW_PUSH},
  [_("dup", 3)] = {"dup", KW_DUP},
  [_("add", 3)] = {"add", KW_ADD},
  [_("sub", 3)] = {"sub", KW_SUB},
  [_("mul", 3)] = {"mul", KW_MUL},
  [_("div", 3)] = {"div", KW_DIV},
  [_("eq", 2)] = {"eq", KW_EQ},
  [_("jmp", 3)] = {"jmp", KW_JMP},
  [_("jz", 2)] = {"jz", KW_JZ},
  [_("jnz", 3)] = {"jnz", KW_JNZ},
  [_("halt", 4)] = {"halt", KW_HALT},
};

token_t lx_maybe_keyword(const char *str, size_t length)
{
  Kw index = KEYWORD_MAP[_(str, length)];
  if (index.name != NULL && strncmp(str, index.name, length) == 0)
    return index.repr;
  else return TOKEN_IDENTIFIER;
}
#undef _

const size_t KEYWORD_COUNT = sizeof((uint8_t[]){_LEXER_KEYWORDS});
const token_t KEYWORDS[KEYWORD_COUNT] = {_LEXER_KEYWORDS};

#define _PARSER_IMPL
#include "parser.h"

#include "vm.h"
#include "disk.h"

typedef struct {
  char *key;
  size_t value;
} Label;

typedef struct {
  char *name;
  size_t ins;
} LabelRef;

typedef struct {
  Inst *buffer;
  size_t ip;
  size_t instcap;
  Label *labels;
  LabelRef *unresolved;
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
  if (c->ip == c->instcap) {
    c->buffer = reallocf(c->buffer, c->instcap *= 2 * sizeof(Inst));
    if (c->buffer == NULL) exit(1);
  }
  c->buffer[c->ip++] = instruction;
}

static parse_error_t parse_int_operand(Parser *p, Value *out)
{
  Value sign = 1;
  if (parse_peek(p).type == TOKEN_MINUS) {
    (void)parse_advance(p);
    sign = -1;
  }

  Token token = parse_expect(p, TOKEN_INTEGER);
  if (token.type == TOKEN_ERROR) return p->error;
  assert(token.length != 0);

  Value value = 0;
  for (size_t i = 0; i < token.length; i++)
    value = value * 10 + (token.start[i] - '0');
  *out = value * sign;

  return PARSE_ERR_NONE;
}

static parse_error_t instruction(Ctx *c, Parser *p)
{
  Token token = parse_expect_one_of(p, KEYWORDS, KEYWORD_COUNT);
  if (token.type == TOKEN_ERROR) return p->error;

  Inst instruction = {0};
  instruction.type = kw_to_inst_t(token.type);
  if (VM_INST_HAS_OP[instruction.type]) {
    switch ((uint8_t)instruction.type) {
    case INST_JMP:
    case INST_JZ:
    case INST_JNZ: {
      Token token = parse_expect(p, TOKEN_IDENTIFIER);
      if (token.type == TOKEN_ERROR) return p->error;
      char *ident = strndup(token.start, token.length);
      LabelRef ref = {ident, c->ip};
      (void)arrput(c->unresolved, ref);
    } break;
    default: {
      parse_error_t result = parse_int_operand(p, &instruction.operand);
      if (result != PARSE_ERR_NONE) return p->error;
    } break;
    }
  }

  Token term = parse_expect(p, TOKEN_NEWLINE);
  if (term.type == TOKEN_ERROR) return p->error;

  ctx_ins(c, instruction);

  return PARSE_ERR_NONE;
}

static parse_error_t label(Ctx *c, Parser *p)
{
  Token token = parse_expect(p, TOKEN_IDENTIFIER);
  if (token.type == TOKEN_ERROR) return p->error;
  Token colon = parse_expect(p, TOKEN_COLON);
  if (colon.type == TOKEN_ERROR) return p->error;
  Token term = parse_expect(p, TOKEN_NEWLINE);
  if (term.type == TOKEN_ERROR) return p->error;
  
  char *ident = strndup(token.start, token.length);
  // TODO: error reporting?
  if (shgeti(c->labels, ident) != -1) return PARSE_ERR_UNEXPECTED_TOKEN;
  shput(c->labels, ident, c->ip);
  return PARSE_ERR_NONE;
}

static parse_error_t label_or_ins(Ctx *c, Parser *p)
{
  const token_t allowed[1 + KEYWORD_COUNT] = {TOKEN_IDENTIFIER, _LEXER_KEYWORDS};
  Token peek = parse_peek_one_of(p, allowed, 1 + KEYWORD_COUNT);
  if (peek.type == TOKEN_ERROR) return p->error;
  else if (peek.type == TOKEN_IDENTIFIER) return label(c, p);
  else return instruction(c, p);
}

const char *const OUT_FILE_EXT = ".ins";

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
    .ip = 0,
    .instcap = 128,
    .labels = NULL,
  };

  for (;;) {
    if (parse_peek(&p).type == TOKEN_EOF) break;
    parse_error_t result = label_or_ins(&ctx, &p);
    if (result != PARSE_ERR_NONE) {
      printf("ERROR: error while compiling instruction: %u\n", result);
      return 1;
    }
  }

  size_t length = arrlenu(ctx.unresolved);
  for (size_t i = 0; i < length; i++) {
    LabelRef ref = ctx.unresolved[i];
    Inst *inst = &ctx.buffer[ref.ins];
    Label *label = shgetp_null(ctx.labels, ref.name);
    if (label == NULL) {
      printf("ERROR: error while resolving labels: %s is not defined", ref.name);
      return 1;
    }
    inst->operand = (int64_t)label->value - (int64_t)ref.ins;
  }

  const char *output = derive_out_path(argv[1]);
  save_prog_to_disk(output, ctx.buffer, ctx.ip);
  return 0;
}
