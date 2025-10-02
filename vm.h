#ifndef VM_H
#define VM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define VM_STACK_CAPACITY 1024

typedef int64_t Value;

#define inst_push(value) (Inst){INST_PUSH,(value)}
#define inst_dup(value)  (Inst){INST_DUP, (value)}
#define inst_add         (Inst){INST_ADD, 0}
#define inst_sub         (Inst){INST_SUB, 0}
#define inst_mul         (Inst){INST_MUL, 0}
#define inst_div         (Inst){INST_DIV, 0}
#define inst_eq          (Inst){INST_EQ, 0}
#define inst_jmp(value)  (Inst){INST_JMP,(value)}
#define inst_jz(value)   (Inst){INST_JZ,(value)}
#define inst_jnz(value)  (Inst){INST_JNZ,(value)}
#define inst_halt        (Inst){INST_HALT, 0}

typedef enum {
  INST_NOP = 0,
  INST_PUSH,
  INST_DUP,
  INST_ADD,
  INST_SUB,
  INST_MUL,
  INST_DIV,
  INST_EQ,
  INST_JMP,
  INST_JZ,
  INST_JNZ,
  INST_HALT = 255,
} inst_t;

typedef struct {
  inst_t type;
  Value operand;
} Inst;

typedef struct {
  Inst *code;
  size_t ip;
  Value stack[VM_STACK_CAPACITY];
  size_t sp;
  bool halted;
} VM;

typedef enum {
  VM_ERR_NONE = 0,
  VM_ERR_STACK_UNDERFLOW,
  VM_ERR_STACK_OVERFLOW,
  VM_ERR_ILLEGAL_INST,
} err_t;

const char* vm_err_to_cstr(err_t error);

void dump_stack(VM *vm);
err_t vm_exec(VM *vm, Inst);
err_t vm_run(VM *vm);

#endif
