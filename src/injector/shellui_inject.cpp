#include "shellui_inject.h"
#include "pt.h"
#include "elfldr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#if defined(__PS5__) || defined(PS5)
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <ps5/kernel.h>
#include <ps5/nid.h>

#define PTRACE_AUTHID 0x4800000000010003ULL

typedef struct {
    void* (*sceKernelDebugOutText)(int channel, const char *msg);
    void* (*elf_main)(void* payload_args);
    void* payload_args;
    int (*pthread_create_ptr)(pthread_t *, const void*, void*(*)(void*), void*);
} SCEFunctions;

static int __attribute__((section(".stager_shellcode$1"))) stager(SCEFunctions* functions) {
    pthread_t thread;
    functions->pthread_create_ptr(&thread, 0, (void*(*)(void*))functions->elf_main, functions->payload_args);
    asm("int3");
    return 0;
}

static int __attribute__((section(".stager_shellcode$2"))) stager_end() {
    return 0;
}

static uint32_t get_stager_size() {
    return (uint32_t)((uintptr_t)&stager_end - (uintptr_t)&stager);
}

pid_t shellui_find_pid(void) {
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0};
    size_t buf_size = 0;

    if (sysctl(mib, 4, NULL, &buf_size, NULL, 0) < 0 || buf_size == 0) {
        return -1;
    }

    std::vector<char> buf(buf_size);
    if (sysctl(mib, 4, buf.data(), &buf_size, NULL, 0) < 0) {
        return -1;
    }

    for (char *ptr = buf.data(); ptr < buf.data() + buf_size;) {
        struct kinfo_proc *ki = reinterpret_cast<struct kinfo_proc *>(ptr);
        if (ki->ki_structsize <= 0) {
            break;
        }
        ptr += ki->ki_structsize;

        if (ki->ki_comm && (strcmp(ki->ki_comm, "SceShellUI") == 0 ||
                            strcmp(ki->ki_comm, "SceShellUI.elf") == 0)) {
            return ki->ki_pid;
        }
    }

    return -1;
}

bool shellui_is_injected(void) {
    return (access("/system_tmp/ps5_overlay_ready", F_OK) == 0);
}

bool shellui_inject_elf(pid_t shellui_pid, const uint8_t *elf_data, size_t elf_size) {
    if (shellui_pid <= 1 || !elf_data || elf_size == 0) {
        fprintf(stderr, "[INJECT] Invalid arguments (PID=%d, size=%zu)\n", shellui_pid, elf_size);
        return false;
    }

    printf("[INJECT] Preparing to inject overlay into SceShellUI (PID: %d)...\n", shellui_pid);

    uint64_t original_authid = kernel_get_ucred_authid(getpid());
    kernel_set_ucred_authid(getpid(), PTRACE_AUTHID);

    if (pt_attach(shellui_pid) < 0) {
        perror("[INJECT] pt_attach failed");
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }
    printf("[INJECT] Attached to SceShellUI (PID: %d)\n", shellui_pid);

    char nid[12] = {0};
    nid_encode("pthread_create", nid);
    intptr_t remote_pthread_create = pt_resolve(shellui_pid, nid);
    if (!remote_pthread_create) {
        fprintf(stderr, "[INJECT] Failed to resolve pthread_create in target!\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }

    nid_encode("sceKernelDebugOutText", nid);
    intptr_t remote_debug_out = pt_resolve(shellui_pid, nid);

    printf("[INJECT] Loading ELF into target address space...\n");
    intptr_t entry = elfldr_load(shellui_pid, const_cast<uint8_t*>(elf_data));
    if (entry <= 0) {
        fprintf(stderr, "[INJECT] elfldr_load failed!\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }
    printf("[INJECT] Target ELF entrypoint at: %#lx\n", (unsigned long)entry);

    intptr_t args = elfldr_payload_args(shellui_pid);
    if (args <= 0) {
        fprintf(stderr, "[INJECT] Failed to allocate payload args\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }

    uint64_t shellcode_size = get_stager_size();
    if (shellcode_size == 0) shellcode_size = 64;

    uint64_t bootstrap = pt_mmap(shellui_pid, 0, shellcode_size,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (!bootstrap || bootstrap == (uint64_t)-1) {
        fprintf(stderr, "[INJECT] Failed to allocate bootstrap stager\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }

    if (kernel_mprotect(shellui_pid, bootstrap, shellcode_size,
                        PROT_EXEC | PROT_WRITE | PROT_READ) != 0) {
        fprintf(stderr, "[INJECT] Failed to mprotect bootstrap stager\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }

    pt_copyin(shellui_pid, (const void*)&stager, bootstrap, shellcode_size);

    SCEFunctions sce_functions{};
    sce_functions.sceKernelDebugOutText = (void*(*)(int, const char*))remote_debug_out;
    sce_functions.pthread_create_ptr = (int(*)(pthread_t*, const void*, void*(*)(void*), void*))remote_pthread_create;
    sce_functions.elf_main = (void*(*)(void*))entry;
    sce_functions.payload_args = (void*)args;

    uint64_t sce_ptr_mem = pt_mmap(shellui_pid, 0, sizeof(sce_functions),
                                   PROT_READ | PROT_WRITE,
                                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (!sce_ptr_mem || sce_ptr_mem == (uint64_t)-1) {
        fprintf(stderr, "[INJECT] Failed to allocate SCEFunctions block\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }
    pt_copyin(shellui_pid, &sce_functions, sce_ptr_mem, sizeof(sce_functions));

    printf("[INJECT] Invoking bootstrap stager...\n");
    if (pt_call2(shellui_pid, bootstrap, sce_ptr_mem) == -1) {
        fprintf(stderr, "[INJECT] pt_call2 failed!\n");
        pt_detach(shellui_pid, 0);
        kernel_set_ucred_authid(getpid(), original_authid);
        return false;
    }

    pt_detach(shellui_pid, 0);
    kernel_set_ucred_authid(getpid(), original_authid);

    printf("[INJECT] Injection complete! Overlay thread spawned inside SceShellUI.\n");
    return true;
}

#else
/* Host stub */
pid_t shellui_find_pid(void) { return 1234; }
bool shellui_is_injected(void) { return false; }
bool shellui_inject_elf(pid_t shellui_pid, const uint8_t *elf_data, size_t elf_size) {
    (void)shellui_pid; (void)elf_data; (void)elf_size;
    printf("[INJECT-MOCK] Injected %zu bytes into SceShellUI\n", elf_size);
    return true;
}
#endif
