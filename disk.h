#ifndef _DISK_H
#define _DISK_H

#include "vm.h"

Inst *copy_prog(const Inst *instructions, size_t count);
void save_bytes_to_disk(const char *path, const uint8_t *bytes, size_t count);
uint8_t *load_bytes_from_disk(const char *path, size_t *nread);
void save_prog_to_disk(const char *path, Inst *instructions, size_t count);
Inst *load_prog_from_disk(const char *path, size_t *readc_out);

#endif
