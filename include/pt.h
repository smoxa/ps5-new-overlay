#pragma once

#include <stdint.h>
#include <sys/types.h>

#if defined(__PS5__) || defined(PS5)
#include <machine/reg.h>
#else
struct reg {
    uint64_t r_r15, r_r14, r_r13, r_r12, r_r11, r_r10, r_r9, r_r8;
    uint64_t r_rdi, r_rsi, r_rbp, r_rbx, r_rdx, r_rcx, r_rax;
    uint32_t r_trapno, r_fs, r_gs, r_err;
    uint64_t r_rip, r_cs, r_rflags, r_rsp, r_ss;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

int pt_attach(pid_t pid);
int pt_detach(pid_t pid, int sig);
int pt_step(int pid);
int pt_continue(pid_t pid, int sig);

int pt_getregs(pid_t pid, struct reg *r);
int pt_setregs(pid_t pid, const struct reg *r);

int pt_copyin(pid_t pid, const void* buf, intptr_t addr, size_t len);
int pt_copyout(pid_t pid, intptr_t addr, void* buf, size_t len);

int pt_getint(pid_t pid, intptr_t addr);
int pt_setint(pid_t pid, intptr_t addr, int val);

long pt_call(pid_t pid, intptr_t addr, ...);
long pt_call2(pid_t pid, intptr_t addr, ...);
long pt_syscall(pid_t pid, int sysno, ...);
intptr_t pt_resolve(pid_t pid, const char* nid);

intptr_t pt_mmap(pid_t pid, intptr_t addr, size_t len, int prot, int flags, int fd, off_t off);
int pt_msync(pid_t pid, intptr_t addr, size_t len, int flags);
int pt_munmap(pid_t pid, intptr_t addr, size_t len);
int pt_mprotect(pid_t pid, intptr_t addr, size_t len, int prot);

int pt_socket(pid_t pid, int domain, int type, int protocol);
int pt_setsockopt(pid_t pid, int fd, int level, int optname, intptr_t optval, uint32_t optlen);
int pt_pipe(pid_t pid, intptr_t pipefd);

#ifdef __cplusplus
}
#endif
