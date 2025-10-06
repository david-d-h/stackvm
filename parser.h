#ifndef _PARSER_H
#define _PARSER_H

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include "lexer.h"

typedef enum {
  PARSE_ERR_NONE = 0,
  PARSE_ERR_NO_STR_TERM,
  PARSE_ERR_UNEXPECTED_TOKEN,
  PARSE_ERR_OUT_OF_TOKENS,
} parse_error_t;

typedef struct {
  bool present;
  Token token;
} Peek;

typedef struct {
  Lexer *lx;
  Peek peek;
  parse_error_t error;
  bool at_eof;
} Parser;

Token parse_advance(Parser *parser);
Token parse_peek(Parser *parser);
Token parse_peek_one_of(Parser *parser, const token_t allowed[], size_t count);
Token parse_expect_one_of(Parser *parser, const token_t allowed[], size_t count);
Token parse_expect(Parser* parser, token_t expected);
parse_error_t parse_skipinc(Parser *parser, token_t term);

#endif

#ifdef _PARSER_IMPL
#undef _PARSER_IMPL

static inline Token _next_token(Parser *parser)
{
  static_assert(TOKEN_ERROR == 0, "the parser assumes TOKEN_ERROR == 0\n");
  Token token = lx_next(parser->lx);
  parser->error = (parse_error_t)(PARSE_ERR_OUT_OF_TOKENS * parser->at_eof);
  token.type = (token_t)(token.type * !parser->at_eof);
  parser->at_eof |= token.type == TOKEN_EOF;
  return token;
}

Token parse_advance(Parser *parser)
{
  if (parser->peek.present) {
    parser->peek.present = false;
    return parser->peek.token;
  }

  return _next_token(parser);
}

Token parse_peek(Parser *parser)
{
  if (!parser->peek.present) {
    parser->peek.present = true;
    parser->peek.token = _next_token(parser);
  }

  return parser->peek.token;
}

Token parse_peek_one_of(Parser *parser, const token_t allowed[], size_t count)
{
  assert(count > 0);
  Token peek = parse_peek(parser);
  bool is_allowed = false;
  while (count != 0) is_allowed |= peek.type == allowed[--count];
  parser->error = (parse_error_t)(PARSE_ERR_UNEXPECTED_TOKEN * !is_allowed);
  peek.type = (token_t)(peek.type * is_allowed);
  return peek;
}

Token parse_expect_one_of(Parser *parser, const token_t allowed[], size_t count)
{
  Token token = parse_peek_one_of(parser, allowed, count);
  (void)parse_advance(parser);
  return token;
}

Token parse_expect(Parser *parser, token_t expected)
{
  return parse_expect_one_of(parser, &expected, 1);
}

parse_error_t parse_skipinc(Parser *parser, token_t term)
{
  for (;;) {
    Token token = parse_advance(parser);
    if (token.type == TOKEN_EOF)
      return parser->error = PARSE_ERR_OUT_OF_TOKENS;
    if (token.type == TOKEN_ERROR) return parser->error;
    if (token.type == term) return PARSE_ERR_NONE;
  }
}

#endif
