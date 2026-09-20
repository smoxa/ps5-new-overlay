#pragma once

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Find the PID of the active SceShellUI process via sysctl(CTL_KERN, KERN_PROC, KERN_PROC_PROC). */
pid_t shellui_find_pid(void);

/** Check whether the overlay is already active/injected into SceShellUI. */
bool shellui_is_injected(void);

/** Inject an ELF binary into SceShellUI using ptrace (kstuff elevated). */
bool shellui_inject_elf(pid_t shellui_pid, const uint8_t *elf_data, size_t elf_size);

#ifdef __cplusplus
}
#endif
