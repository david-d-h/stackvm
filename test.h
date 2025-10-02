#ifndef _TEST_H
#define _TEST_H

#include <stddef.h>
#include <stdbool.h>

#ifndef _TEST_MAX_TESTS
#define _TEST_MAX_TESTS 1024
#endif

typedef struct {
  const char* name;
  const char* file;
  void(*impl)(void);
} Test;

#define t_asserteq(a, b) \
  t_assert_msg((a) == (b), #a" != "#b)

#define t_assert(cond) \
  t_assert_msg(cond, #cond)

void t_assert_msg(bool cond, const char *message);

#endif // _TEST_H

#ifndef _TEST_GENERATED
#define _TEST_GENERATED
#ifdef _TEST_IMPL

#include <stdio.h>
#include <stdlib.h>

Test _TEST_TESTCASES[_TEST_MAX_TESTS] = {0};
size_t _test_num_testcases = 0;

void t_assert_msg(bool cond, const char* message)
{
  if (cond) return;
  printf("assertion failed: %s\n", message);
  exit(1);
}

#define test(name_) \
  void test_##name_(void); \
  __attribute__((constructor)) void _test_register_test_##name_(void) { \
    if (_test_num_testcases > _TEST_MAX_TESTS) { \
      fprintf(stderr, "MAXIMUM AMOUNT OF TESTS REACHED, CONSIDER CHANGING _TEST_MAX_TESTS...\n"); \
      exit(1); \
    } \
    _TEST_TESTCASES[_test_num_testcases++] = (Test){.name = #name_, .file = __FILE_NAME__, .impl = test_##name_}; \
  } \
  void test_##name_(void)
#endif // _TEST_IMPL
#endif // _TEST_GENERATED
