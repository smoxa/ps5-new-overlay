#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Load ELF image into target process; returns entrypoint address or 0 on failure. */
intptr_t elfldr_load(pid_t pid, uint8_t *elf);

/** Allocate payload_args_t-compatible block in target process. Returns address or 0. */
intptr_t elfldr_payload_args(pid_t pid);

/** Basic ELF validation. Returns 0 when valid. */
int elfldr_sanity_check(uint8_t *elf, size_t elf_size);

#ifdef __cplusplus
}
#endif
