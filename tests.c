#define _TEST_IMPL
#include "test.h"

#include <stdio.h>

#define _LEXER_IMPL
#include "lexer.h"

int main(void) {
  for (size_t i = 0; i < _test_num_testcases; i++) {
    Test test = _TEST_TESTCASES[i];
    printf("executing test %s:%s...\n", test.file, test.name);
    test.impl();
  }
  printf("executed all tests successfully...\n");
  return 0;
}
