#include "elfldr.h"
#include "pt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if defined(__PS5__) || defined(PS5)
#include <elf.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ps5/kernel.h>

#ifndef IPV6_2292PKTOPTIONS
#define IPV6_2292PKTOPTIONS 25
#endif

#define ROUND_PG(x) (((x) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))
#define TRUNC_PG(x) ((x) & ~(PAGE_SIZE - 1))
#define PFLAGS(x)   ((((x) & PF_R) ? PROT_READ  : 0) | \
                     (((x) & PF_W) ? PROT_WRITE : 0) | \
                     (((x) & PF_X) ? PROT_EXEC  : 0))

typedef struct elfldr_ctx {
    uint8_t* elf;
    pid_t    pid;
    intptr_t base_addr;
    size_t   base_size;
    void*    base_mirror;
} elfldr_ctx_t;

int elfldr_sanity_check(uint8_t *elf, size_t elf_size) {
    if (!elf || elf_size < sizeof(Elf64_Ehdr)) {
        return -1;
    }
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)elf;
    if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L'  || ehdr->e_ident[3] != 'F') {
        return -1;
    }
    return 0;
}

static int r_relative(elfldr_ctx_t *ctx, Elf64_Rela* rela) {
    intptr_t* loc = (intptr_t*)(ctx->base_mirror + rela->r_offset);
    intptr_t val = ctx->base_addr + rela->r_addend;
    *loc = val;
    return 0;
}

static int data_load(elfldr_ctx_t *ctx, Elf64_Phdr *phdr) {
    void* data = ctx->base_mirror + phdr->p_vaddr;
    if (!phdr->p_memsz) return 0;
    memset(data, 0, phdr->p_memsz);
    if (!phdr->p_filesz) return 0;
    memcpy(data, ctx->elf + phdr->p_offset, phdr->p_filesz);
    return 0;
}

intptr_t elfldr_load(pid_t pid, uint8_t *elf) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)elf;
    Elf64_Phdr *phdr = (Elf64_Phdr*)(elf + ehdr->e_phoff);
    Elf64_Shdr *shdr = (Elf64_Shdr*)(elf + ehdr->e_shoff);

    elfldr_ctx_t ctx = { .elf = elf, .pid = pid };
    size_t min_vaddr = (size_t)-1;
    size_t max_vaddr = 0;
    int error = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_LOAD) continue;
        if (phdr[i].p_vaddr < min_vaddr) min_vaddr = phdr[i].p_vaddr;
        if (max_vaddr < phdr[i].p_vaddr + phdr[i].p_memsz) {
            max_vaddr = phdr[i].p_vaddr + phdr[i].p_memsz;
        }
    }

    min_vaddr = TRUNC_PG(min_vaddr);
    max_vaddr = ROUND_PG(max_vaddr);
    ctx.base_size = max_vaddr - min_vaddr;

    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    int prot = PROT_READ | PROT_WRITE;
    if (ehdr->e_type == ET_DYN) {
        ctx.base_addr = 0;
    } else if (ehdr->e_type == ET_EXEC) {
        ctx.base_addr = min_vaddr;
        flags |= MAP_FIXED;
    } else {
        fprintf(stderr, "[ELFLDR] Unsupported ELF type %d\n", ehdr->e_type);
        return 0;
    }

    ctx.base_mirror = malloc(ctx.base_size);
    if (!ctx.base_mirror) {
        perror("[ELFLDR] malloc base_mirror failed");
        return 0;
    }

    ctx.base_addr = pt_mmap(pid, ctx.base_addr, ctx.base_size, prot, flags, -1, 0);
    if (ctx.base_addr == -1) {
        perror("[ELFLDR] pt_mmap failed");
        free(ctx.base_mirror);
        return 0;
    }

    for (int i = 0; i < ehdr->e_phnum && !error; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            error = data_load(&ctx, &phdr[i]);
        }
    }

    for (int i = 0; i < ehdr->e_shnum && !error; i++) {
        if (shdr[i].sh_type != SHT_RELA) continue;
        Elf64_Rela* rela = (Elf64_Rela*)(elf + shdr[i].sh_offset);
        for (size_t j = 0; j < shdr[i].sh_size / sizeof(Elf64_Rela); j++) {
            if ((rela[j].r_info & 0xffffffffL) == R_X86_64_RELATIVE) {
                error = r_relative(&ctx, &rela[j]);
            }
        }
    }

    if (pt_copyin(ctx.pid, ctx.base_mirror, ctx.base_addr, ctx.base_size)) {
        perror("[ELFLDR] pt_copyin failed");
        error = 1;
    }

    for (int i = 0; i < ehdr->e_phnum && !error; i++) {
        if (phdr[i].p_type != PT_LOAD || phdr[i].p_memsz == 0) continue;
        if (pt_mprotect(pid, ctx.base_addr + phdr[i].p_vaddr,
                        ROUND_PG(phdr[i].p_memsz),
                        PFLAGS(phdr[i].p_flags))) {
            perror("[ELFLDR] pt_mprotect failed");
            error = 1;
        }
    }

    pt_msync(pid, ctx.base_addr, ctx.base_size, MS_SYNC);
    free(ctx.base_mirror);

    if (error) {
        pt_munmap(pid, ctx.base_addr, ctx.base_size);
        return 0;
    }

    return ctx.base_addr + ehdr->e_entry;
}

intptr_t elfldr_payload_args(pid_t pid) { (void)pid; return 0; }
#else
int elfldr_sanity_check(uint8_t *elf, size_t elf_size) { (void)elf; (void)elf_size; return 0; }
intptr_t elfldr_load(pid_t pid, uint8_t *elf) { (void)pid; (void)elf; return 0; }
intptr_t elfldr_payload_args(pid_t pid) { (void)pid; return 0; }
#endif
