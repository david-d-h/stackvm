#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include "disk.h"
#include "vm.h"

#define __BSWAP_ON __ORDER_BIG_ENDIAN__

#if __BYTE_ORDER__ == __BSWAP_ON
static inline void _bswap_inst_in_place(Inst *instruction)
{
  static_assert(sizeof(inst_t) == 4, "_bswap_inst_in_place assumes inst_t is repr uint32_t");
  instruction->type = __builtin_bswap32(instruction->type);
  instruction->operand = __builtin_bswap64(instruction->operand);
}
#endif

Inst *copy_prog(const Inst *instructions, size_t count)
{
  Inst *buffer = malloc(sizeof(Inst) * count);
  return memcpy(buffer, instructions, sizeof(Inst) * count);
}

#define _DISK_IO_ERROR(msg) \
  do {                \
    errmsg = (msg);   \
    goto IO_ERROR;    \
  } while (0)

void save_bytes_to_disk(const char *path, const uint8_t *bytes, size_t count)
{
  const char* errmsg = NULL;

  FILE *file = fopen(path, "wb");
  if (file == NULL) _DISK_IO_ERROR("could not open file");
  const size_t written = fwrite(bytes, 1, count, file);
  if (written != count) _DISK_IO_ERROR("while writing to file");
  return;

IO_ERROR:
  fprintf(stderr, "Error: (operation on %s) %s: %s", path, errmsg, strerror(errno));
  exit(1);
}

uint8_t *load_bytes_from_disk(const char* path, size_t *nread)
{
  const char* errmsg = NULL;
  
  FILE *file = fopen(path, "rb");
  if (file == NULL) _DISK_IO_ERROR("could not open file");
  if (fseek(file, 0L, SEEK_END) != 0) _DISK_IO_ERROR("while seeking for EOF");
  int length = ftell(file);
  if (length < 0) _DISK_IO_ERROR("while trying to determine position of FD");
  (void)fseek(file, 0L, SEEK_SET);
  uint8_t *buffer = malloc(length);
  if (buffer == NULL) exit(1);
  const size_t nread_ = fread(buffer, 1, length, file);
  if (nread_ != (size_t)length) _DISK_IO_ERROR("while trying to read whole file");
  *nread = nread_;
  return buffer;

IO_ERROR:
  fprintf(stderr, "Error: (operation on %s) %s: %s", path, errmsg, strerror(errno));
  exit(1);
}

void save_prog_to_disk(const char *path, Inst* const instructions,
                       size_t count)
{
#if __BYTE_ORDER__ == __BSWAP_ON
  for (size_t i = 0; i < count; i++)
    _bswap_inst_in_place(instructions + i);
#endif
  save_bytes_to_disk(path, (uint8_t *)instructions, sizeof(Inst) * count);
}

Inst *load_prog_from_disk(const char *path, size_t *const nread)
{
  // const char* errmsg = NULL;

  size_t nread_;
  Inst *buffer = (Inst *)load_bytes_from_disk(path, &nread_);
  assert(nread_ % sizeof(Inst) == 0);
  *nread = nread_ / sizeof(Inst);
#if __BYTE_ORDER__ == __BSWAP_ON
  for (size_t i = 0; i < nread_; i++)
    _bswap_inst_in_place(buffer + i);
#endif
  return buffer;

//   FILE *file = fopen(path, "rb");
//   if (file == NULL) _DISK_IO_ERROR("could not open file");
//   if (fseek(file, 0L, SEEK_END) != 0) _DISK_IO_ERROR("while seeking for EOF");
//   int length = ftell(file);
//   if (length < 0) _DISK_IO_ERROR("while trying to determine position of FD");
//   (void)fseek(file, 0L, SEEK_SET);
//   assert(length % sizeof(Inst) == 0);
//   Inst *buffer = malloc(length);
//   if (buffer == NULL) exit(1);
//   const size_t nread_ = fread(buffer, sizeof(Inst), length / sizeof(Inst), file);
//   if (length / sizeof(Inst) != nread_) _DISK_IO_ERROR("while trying to read whole file");
//   *nread = nread_;
// #if __BYTE_ORDER__ == __BSWAP_ON
//   for (size_t i = 0; i < nread_; i++)
//     _bswap_inst_in_place(buffer + i);
// #endif
//   return buffer;

// IO_ERROR:
//   fprintf(stderr, "Error: (operation on %s) %s: %s", path, errmsg, strerror(errno));
//   exit(1);
}

#undef IO_ERROR
