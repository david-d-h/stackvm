#include <stdio.h>

#include "vm.h"
#include "disk.h"

const char *filepath = "examples/negative.ins";

int main(int argc, const char *argv[])
{
  if (argc != 2) {
    fprintf(stderr,
            "Error: expected path to bytecode file\n"
            "Usage: %s <filepath>",
            argv[0]);
    return 1;
  }

  size_t nread;
  Inst *code = load_prog_from_disk(argv[1], &nread);
  VM vm = {0};
  vm.code = code;

  vm_err_t result = vm_run(&vm);
  if (result != VM_ERR_NONE) {
    printf("Error while interpreting %s: %s\n",
           filepath,
           vm_err_to_cstr(result));
    return 1;
  }

  dump_stack(&vm);
  return 0;
}
