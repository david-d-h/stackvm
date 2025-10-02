#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "vm.h"

const char* vm_err_to_cstr(err_t error)
{
  switch (error) {
  case VM_ERR_NONE: return "no error";
  case VM_ERR_STACK_UNDERFLOW: return "stack underflow";
  case VM_ERR_STACK_OVERFLOW: return "stack overflow";
  case VM_ERR_ILLEGAL_INST: return "encountered illegal instruction";
  }
}

#define __binop(operation)                                                         \
  do {                                                                             \
    if (vm->sp < 2) return VM_ERR_STACK_UNDERFLOW;                                    \
    vm->stack[vm->sp - 2] = vm->stack[vm->sp - 1] operation vm->stack[vm->sp - 2]; \
    vm->sp--;                                                                      \
  } while (0)

void dump_stack(VM *vm)
{
  printf("STACK DUMP:\n");
  for (size_t i = 0; i < vm->sp; i++) {
    printf("  %lld\n", vm->stack[i]);
  }
}

err_t vm_exec(VM *vm, Inst inst)
{
  switch (inst.type) {
  case INST_NOP: break;

  case INST_PUSH: {
    if (vm->sp >= VM_STACK_CAPACITY)
      return VM_ERR_STACK_OVERFLOW;
    vm->stack[vm->sp++] = inst.operand;
  } break;

  case INST_DUP: {
    if (vm->sp - inst.operand <= 0) return VM_ERR_STACK_UNDERFLOW;
    if (vm->sp >= VM_STACK_CAPACITY) return VM_ERR_STACK_OVERFLOW;
    vm->stack[vm->sp] = vm->stack[vm->sp - 1 - inst.operand];
    vm->sp++;
  } break;

  case INST_ADD: __binop(+); break;
  case INST_SUB: __binop(-); break;
  case INST_MUL: __binop(*); break;
  case INST_DIV: __binop(/); break;
  case INST_EQ: __binop(==); break;

  case INST_JMP: {
    vm->ip += inst.operand;
  } return VM_ERR_NONE;

  case INST_JZ: {
    if (vm->sp == 0) return VM_ERR_STACK_UNDERFLOW;
    if ((bool)vm->stack[vm->sp--]) break;
    vm->ip += inst.operand;
    return VM_ERR_NONE;
  };

  case INST_JNZ: {
    if (vm->sp == 0) return VM_ERR_STACK_UNDERFLOW;
    if (!(bool)vm->stack[vm->sp--]) break;
    vm->ip += inst.operand;
    return VM_ERR_NONE;
  };

  case INST_HALT: {
    vm->halted = true;
  } break;

  default: return VM_ERR_ILLEGAL_INST;
  }
  vm->ip++;
  return VM_ERR_NONE;
}

#undef __binop

err_t vm_run(VM *vm)
{
  while (!vm->halted) {
    err_t result = vm_exec(vm, vm->code[vm->ip]);
    if (result != VM_ERR_NONE) return result;
  }
  return VM_ERR_NONE;
}
