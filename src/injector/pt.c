#include "pt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#if defined(__PS5__) || defined(PS5)
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <ps5/kernel.h>

static uintptr_t pt_private_entry_rsp(uintptr_t interrupted_rsp) {
    return (interrupted_rsp & ~(uintptr_t)0xf) - (uintptr_t)0x108;
}

static int sys_ptrace(int request, pid_t pid, caddr_t addr, int data) {
    return (int)syscall(SYS_ptrace, request, pid, addr, data);
}

intptr_t pt_resolve(pid_t pid, const char* nid) {
    (void)pid;
    (void)nid;
    return 0;
}

int pt_attach(pid_t pid) {
    int status = 0;
    if (sys_ptrace(PT_ATTACH, pid, 0, 0) == -1) {
        return -1;
    }
    if (waitpid(pid, &status, WUNTRACED) == -1) {
        return -1;
    }
    if (!WIFSTOPPED(status)) {
        errno = ESRCH;
        return -1;
    }
    return 0;
}

int pt_detach(pid_t pid, int sig) {
    if (sys_ptrace(PT_DETACH, pid, 0, sig) == -1) {
        return -1;
    }
    return 0;
}

int pt_step(int pid) {
    if (sys_ptrace(PT_STEP, pid, (caddr_t)1, 0)) {
        return -1;
    }
    if (waitpid(pid, 0, 0) < 0) {
        return -1;
    }
    return 0;
}

int pt_continue(pid_t pid, int sig) {
    if (sys_ptrace(PT_CONTINUE, pid, (caddr_t)1, sig) == -1) {
        return -1;
    }
    return 0;
}

int pt_getregs(pid_t pid, struct reg *r) {
    return sys_ptrace(PT_GETREGS, pid, (caddr_t)r, 0);
}

int pt_setregs(pid_t pid, const struct reg *r) {
    return sys_ptrace(PT_SETREGS, pid, (caddr_t)r, 0);
}

int pt_copyin(pid_t pid, const void* buf, intptr_t addr, size_t len) {
    struct ptrace_io_desc iod = {
        .piod_op = PIOD_WRITE_D,
        .piod_offs = (void*)addr,
        .piod_addr = (void*)buf,
        .piod_len = len
    };
    return sys_ptrace(PT_IO, pid, (caddr_t)&iod, 0);
}

int pt_copyout(pid_t pid, intptr_t addr, void* buf, size_t len) {
    struct ptrace_io_desc iod = {
        .piod_op = PIOD_READ_D,
        .piod_offs = (void*)addr,
        .piod_addr = buf,
        .piod_len = len
    };
    return sys_ptrace(PT_IO, pid, (caddr_t)&iod, 0);
}

int pt_getint(pid_t pid, intptr_t addr) {
    return sys_ptrace(PT_READ_D, pid, (caddr_t)addr, 0);
}

int pt_setint(pid_t pid, intptr_t addr, int val) {
    return sys_ptrace(PT_WRITE_D, pid, (caddr_t)addr, val);
}

long pt_call(pid_t pid, intptr_t addr, ...) {
    struct reg jmp_reg;
    struct reg bak_reg;
    uintptr_t entry_rsp;
    va_list ap;

    if (pt_getregs(pid, &bak_reg)) {
        return -1;
    }

    memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));
    jmp_reg.r_rip = addr;

    entry_rsp = pt_private_entry_rsp((uintptr_t)bak_reg.r_rsp);
    jmp_reg.r_rsp = entry_rsp;

    va_start(ap, addr);
    jmp_reg.r_rdi = va_arg(ap, uint64_t);
    jmp_reg.r_rsi = va_arg(ap, uint64_t);
    jmp_reg.r_rdx = va_arg(ap, uint64_t);
    jmp_reg.r_rcx = va_arg(ap, uint64_t);
    jmp_reg.r_r8  = va_arg(ap, uint64_t);
    jmp_reg.r_r9  = va_arg(ap, uint64_t);
    va_end(ap);

    if (pt_setregs(pid, &jmp_reg)) {
        return -1;
    }

    while ((uintptr_t)jmp_reg.r_rsp <= entry_rsp) {
        if (pt_step(pid)) {
            return -1;
        }
        if (pt_getregs(pid, &jmp_reg)) {
            return -1;
        }
    }

    if (pt_setregs(pid, &bak_reg)) {
        return -1;
    }

    return jmp_reg.r_rax;
}

long pt_call2(pid_t pid, intptr_t addr, ...) {
    struct reg jmp_reg;
    struct reg bak_reg;
    uintptr_t entry_rsp;
    va_list ap;

    if (pt_getregs(pid, &bak_reg)) {
        return -1;
    }

    memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));
    jmp_reg.r_rip = addr;

    entry_rsp = pt_private_entry_rsp((uintptr_t)bak_reg.r_rsp);
    jmp_reg.r_rsp = entry_rsp;

    va_start(ap, addr);
    jmp_reg.r_rdi = va_arg(ap, uint64_t);
    jmp_reg.r_rsi = va_arg(ap, uint64_t);
    jmp_reg.r_rdx = va_arg(ap, uint64_t);
    jmp_reg.r_rcx = va_arg(ap, uint64_t);
    jmp_reg.r_r8  = va_arg(ap, uint64_t);
    jmp_reg.r_r9  = va_arg(ap, uint64_t);
    va_end(ap);

    if (pt_setregs(pid, &jmp_reg)) {
        return -1;
    }

    if (pt_continue(pid, 0) != 0) {
        return -1;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGTRAP) {
        errno = EPROTO;
        return -1;
    }

    if (pt_setregs(pid, &bak_reg) != 0) {
        return -1;
    }

    return jmp_reg.r_rax;
}

static intptr_t find_remote_syscall(pid_t pid) {
    struct reg r;
    if (pt_getregs(pid, &r) < 0) return 0;
    
    intptr_t start = (r.r_rip & ~0xFFF);
    uint8_t buf[0x2000];
    if (pt_copyout(pid, start, buf, sizeof(buf)) < 0) return 0;
    
    for (size_t i = 0; i < sizeof(buf) - 1; i++) {
        if (buf[i] == 0x0F && buf[i+1] == 0x05) {
            return start + i;
        }
    }
    return 0;
}

long pt_syscall(pid_t pid, int sysno, ...) {
    static intptr_t cached_syscall_addr = 0;
    intptr_t addr = cached_syscall_addr ? cached_syscall_addr : (cached_syscall_addr = find_remote_syscall(pid));
    struct reg jmp_reg;
    struct reg bak_reg;
    uintptr_t entry_rsp;
    va_list ap;

    if (!addr) {
        return -1;
    }
    // addr points directly to `syscall`, no +0xa needed.

    if (pt_getregs(pid, &bak_reg)) {
        return -1;
    }

    memcpy(&jmp_reg, &bak_reg, sizeof(jmp_reg));
    jmp_reg.r_rip = addr;
    jmp_reg.r_rax = sysno;

    entry_rsp = pt_private_entry_rsp((uintptr_t)bak_reg.r_rsp);
    jmp_reg.r_rsp = entry_rsp;

    va_start(ap, sysno);
    jmp_reg.r_rdi = va_arg(ap, uint64_t);
    jmp_reg.r_rsi = va_arg(ap, uint64_t);
    jmp_reg.r_rdx = va_arg(ap, uint64_t);
    jmp_reg.r_r10 = va_arg(ap, uint64_t);
    jmp_reg.r_r8  = va_arg(ap, uint64_t);
    jmp_reg.r_r9  = va_arg(ap, uint64_t);
    va_end(ap);

    if (pt_setregs(pid, &jmp_reg)) {
        return -1;
    }

    while ((uintptr_t)jmp_reg.r_rsp <= entry_rsp) {
        if (pt_step(pid)) {
            return -1;
        }
        if (pt_getregs(pid, &jmp_reg)) {
            return -1;
        }
    }

    if (pt_setregs(pid, &bak_reg)) {
        return -1;
    }

    return jmp_reg.r_rax;
}

intptr_t pt_mmap(pid_t pid, intptr_t addr, size_t len, int prot, int flags, int fd, off_t off) {
    return pt_syscall(pid, SYS_mmap, addr, len, prot, flags, fd, off);
}

int pt_msync(pid_t pid, intptr_t addr, size_t len, int flags) {
    return (int)pt_syscall(pid, SYS_msync, addr, len, flags);
}

int pt_munmap(pid_t pid, intptr_t addr, size_t len) {
    return (int)pt_syscall(pid, SYS_munmap, addr, len);
}

int pt_mprotect(pid_t pid, intptr_t addr, size_t len, int prot) {
    return (int)pt_syscall(pid, SYS_mprotect, addr, len, prot);
}

int pt_socket(pid_t pid, int domain, int type, int protocol) {
    return (int)pt_syscall(pid, SYS_socket, domain, type, protocol);
}

int pt_setsockopt(pid_t pid, int fd, int level, int optname, intptr_t optval, uint32_t optlen) {
    return (int)pt_syscall(pid, SYS_setsockopt, fd, level, optname, optval, optlen, 0);
}

int pt_pipe(pid_t pid, intptr_t pipefd) {
    intptr_t faddr = pt_resolve(pid, "-Jp7F+pXxNg");
    return (int)pt_call(pid, faddr, pipefd);
}

#else
/* Host stubs for non-PS5 build testing */
int pt_attach(pid_t pid) { (void)pid; return -1; }
int pt_detach(pid_t pid, int sig) { (void)pid; (void)sig; return 0; }
int pt_step(int pid) { (void)pid; return 0; }
int pt_continue(pid_t pid, int sig) { (void)pid; (void)sig; return 0; }
int pt_getregs(pid_t pid, struct reg *r) { (void)pid; (void)r; return -1; }
int pt_setregs(pid_t pid, const struct reg *r) { (void)pid; (void)r; return -1; }
int pt_copyin(pid_t pid, const void* buf, intptr_t addr, size_t len) { (void)pid; (void)buf; (void)addr; (void)len; return -1; }
int pt_copyout(pid_t pid, intptr_t addr, void* buf, size_t len) { (void)pid; (void)addr; (void)buf; (void)len; return -1; }
int pt_getint(pid_t pid, intptr_t addr) { (void)pid; (void)addr; return 0; }
int pt_setint(pid_t pid, intptr_t addr, int val) { (void)pid; (void)addr; (void)val; return 0; }
long pt_call(pid_t pid, intptr_t addr, ...) { (void)pid; (void)addr; return 0; }
long pt_call2(pid_t pid, intptr_t addr, ...) { (void)pid; (void)addr; return 0; }
long pt_syscall(pid_t pid, int sysno, ...) { (void)pid; (void)sysno; return 0; }
intptr_t pt_resolve(pid_t pid, const char* nid) { (void)pid; (void)nid; return 0; }
intptr_t pt_mmap(pid_t pid, intptr_t addr, size_t len, int prot, int flags, int fd, off_t off) { (void)pid; (void)addr; (void)len; (void)prot; (void)flags; (void)fd; (void)off; return -1; }
int pt_msync(pid_t pid, intptr_t addr, size_t len, int flags) { (void)pid; (void)addr; (void)len; (void)flags; return 0; }
int pt_munmap(pid_t pid, intptr_t addr, size_t len) { (void)pid; (void)addr; (void)len; return 0; }
int pt_mprotect(pid_t pid, intptr_t addr, size_t len, int prot) { (void)pid; (void)addr; (void)len; (void)prot; return 0; }
int pt_socket(pid_t pid, int domain, int type, int protocol) { (void)pid; (void)domain; (void)type; (void)protocol; return -1; }
int pt_setsockopt(pid_t pid, int fd, int level, int optname, intptr_t optval, uint32_t optlen) { (void)pid; (void)fd; (void)level; (void)optname; (void)optval; (void)optlen; return 0; }
int pt_pipe(pid_t pid, intptr_t pipefd) { (void)pid; (void)pipefd; return -1; }
#endif
