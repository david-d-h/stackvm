#ifndef _LEXER_H
#define _LEXER_H
#include <stddef.h>

typedef enum {
  TOKEN_ERROR = 0,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LBRACK,
  TOKEN_RBRACK,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_SEMICOLON,
  TOKEN_COLON,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_GT,
  TOKEN_GE,
  TOKEN_LT,
  TOKEN_LE,
  TOKEN_EQ,
  TOKEN_EQEQ,
  TOKEN_BANGEQ,
  TOKEN_BANG,
  TOKEN_INTEGER,
  TOKEN_FLOAT,
  TOKEN_STRING,
  TOKEN_IDENTIFIER,
#ifdef _LEXER_KEYWORDS
  _LEXER_KEYWORDS,
#endif
#ifdef _LEXER_TOKENIZE_NEWLINE
  TOKEN_NEWLINE,
#endif
  TOKEN_EOF,
} token_t;

typedef struct {
  token_t type;
  const char *start;
  size_t length;
  size_t line;
} Token;

typedef enum {
  LX_ERR_NONE = 0,
  LX_ERR_NO_STR_TERM,
} lx_err_t;

typedef struct {
  const char *source;
  size_t cursor;
  size_t anchor;
  size_t line;
  lx_err_t last_error;
} Lexer;

extern token_t lx_maybe_keyword(const char *str, size_t length);
Lexer lx_new(const char *source);
Token lx_next(Lexer *lx);

#endif // _LEXER_H

#ifdef _LEXER_IMPL
#undef _LEXER_IMPL
#include <assert.h>
#include <stdint.h>
#include <ctype.h>

static inline Token _lx_finalize(Lexer *lx, token_t type)
{
  Token token = {
    .type = type,
    .start = lx->source + lx->anchor,
    .length = lx->cursor - lx->anchor,
    .line = lx->line,
  };
  lx->anchor = lx->cursor;
  return token;
}

static inline char _lx_peek(Lexer *lx)
{
  return lx->source[lx->cursor];
}

static inline Token _lx_if_peek_eq_else(Lexer *lx,
                                        char check,
                                        token_t then,
                                        token_t otherwise)
{
  if (_lx_peek(lx) == check) {
    lx->cursor++;
    return _lx_finalize(lx, then);
  }

  return _lx_finalize(lx, otherwise);
}

static inline char _lx_advance(Lexer *lx)
{
  return lx->source[lx->cursor++];
}

#ifndef _LEXER_KEYWORDS
inline token_t lx_maybe_keyword(const char *str, size_t length)
{
  (void)str; (void)length;
  return TOKEN_IDENTIFIER;
}
#endif

static Token _lx_number(Lexer *lx)
{
  token_t found_dot = TOKEN_INTEGER;
  for (;;) {
    char byte = _lx_peek(lx);
    if (byte == '.') {
      if (found_dot == TOKEN_FLOAT) break;
      found_dot = TOKEN_FLOAT;
    }
    else if (byte < '0' || '9' < byte) break;
    lx->cursor++;
  }
  return _lx_finalize(lx, found_dot);
}

static Token _lx_ident_or_kw(Lexer *lx)
{
  for (;; lx->cursor++) {
    char byte = _lx_peek(lx);
    if ((byte < 'a' || 'z' < byte) && byte != '_') break;
  }
  token_t type = lx_maybe_keyword(lx->source + lx->anchor,
                                  lx->cursor - lx->anchor);
  return _lx_finalize(lx, type);
}

static Token _lx_string(Lexer *lx)
{
  char byte;
  for (;; lx->cursor++) {
    byte = _lx_peek(lx);
    if (byte == '\0' || byte == '"') break;
  }
  static_assert(LX_ERR_NO_STR_TERM == 1, "_lx_string assumes the string termination error is repr 1");
  // equivalent to (byte == '"' ? TOKEN_STRING : TOKEN_ERROR)
  // because TOKEN_ERROR = 0.
  token_t type = (token_t)(TOKEN_STRING * (byte == '"'));
  lx->last_error = (lx_err_t)(byte == '"');
  return _lx_finalize(lx, type);
}

Lexer lx_new(const char *source)
{
  return (Lexer) {
    .source = source,
    .cursor = 0,
    .anchor = 0,
    .line = 0,
  };
}

Token lx_next(Lexer *lx)
{
WHITESPACE:
  if (_lx_peek(lx) == '\0') return _lx_finalize(lx, TOKEN_EOF);
  while (_lx_peek(lx) == ' ') lx->cursor++;
  if (_lx_peek(lx) == '\n') {
    lx->line++;
    lx->cursor++;
#ifdef _LEXER_TOKENIZE_NEWLINE
    return _lx_finalize(lx, TOKEN_NEWLINE);
#endif
    goto WHITESPACE;
  }

  lx->anchor = lx->cursor;
  char byte = _lx_advance(lx);

  switch (byte) {
  case '(': return _lx_finalize(lx, TOKEN_LPAREN);
  case ')': return _lx_finalize(lx, TOKEN_RPAREN);
  case '[': return _lx_finalize(lx, TOKEN_LBRACK);
  case ']': return _lx_finalize(lx, TOKEN_RBRACK);
  case '{': return _lx_finalize(lx, TOKEN_LBRACE);
  case '}': return _lx_finalize(lx, TOKEN_RBRACE);
  case ',': return _lx_finalize(lx, TOKEN_COMMA);
  case '.': return _lx_finalize(lx, TOKEN_DOT);
  case ';': return _lx_finalize(lx, TOKEN_SEMICOLON);
  case ':': return _lx_finalize(lx, TOKEN_COLON);
  case '+': return _lx_finalize(lx, TOKEN_PLUS);
  case '-': return _lx_finalize(lx, TOKEN_MINUS);
  case '*': return _lx_finalize(lx, TOKEN_STAR);
  case '/': return _lx_finalize(lx, TOKEN_SLASH);
  case '!': return _lx_if_peek_eq_else(lx, '=', TOKEN_BANGEQ, TOKEN_BANG);
  case '=': return _lx_if_peek_eq_else(lx, '=', TOKEN_EQEQ, TOKEN_EQ);
  case '>': return _lx_if_peek_eq_else(lx, '=', TOKEN_GE, TOKEN_GT);
  case '<': return _lx_if_peek_eq_else(lx, '=', TOKEN_LE, TOKEN_LT);
  default:
    if (byte >= '0' && '9' >= byte) return _lx_number(lx);
    else if ((byte >= 'a' && 'z' >= byte) ||
             (byte >= 'A' && 'Z' >= byte)) return _lx_ident_or_kw(lx);
    else if (byte == '"') return _lx_string(lx);
    else return _lx_finalize(lx, TOKEN_ERROR);
  }
}
#endif // _LEXER_IMPL


#ifdef _TEST_IMPL
#include "test.h"

#define assert_tokens(source, ...)           \
  do {                                       \
    const uint8_t tokens[] = {__VA_ARGS__};  \
    const size_t count = sizeof(tokens);     \
    _assert_tokens((source), tokens, count); \
  } while (0)

static void _assert_tokens(const char *source,
                           const uint8_t *expected,
                           size_t count)
{
  Lexer lexer = lx_new(source);
  for (size_t i = 0; i < count; i++) {
    token_t next_token = lx_next(&lexer).type;
    t_asserteq(next_token, expected[i]);
  }
}

test(it_works) {
  assert_tokens("190 + ababaaa_",
                TOKEN_INTEGER, TOKEN_PLUS, TOKEN_IDENTIFIER,
                TOKEN_EOF);
}

test(multiline_asm) {
  assert_tokens("push 1\n"
                "push 2\n"
                "add",
                TOKEN_IDENTIFIER, TOKEN_INTEGER,
                TOKEN_IDENTIFIER, TOKEN_INTEGER,
                TOKEN_IDENTIFIER);
}

#undef assert_tokens
#endif
